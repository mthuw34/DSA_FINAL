// Kiểm tra logic giao diện bằng DOM giả, không chạm vào database người dùng.
const { readFileSync } = require('node:fs');
const { resolve } = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const elements = new Map();
function element(key) {
  if (!elements.has(key)) elements.set(key, {
    innerHTML:'',textContent:'',hidden:false,disabled:false,value:'',open:false,
    classList:{toggle(){}},listeners:{},setAttribute(){},focus(){},setSelectionRange(){},
    addEventListener(type,handler){ this.listeners[type] = handler; },
    showModal(){this.open=true;},close(){this.open=false;}
  });
  return elements.get(key);
}
const calls = [];
const malicious = '<img src=x onerror=alert(1)>';
const fixture = {
  '/api/patients':[{id:1,name:malicious,birth_date:'2000-01-01',age:26,phone:'0901234567',gender:'Nam'},
    {id:2,name:'Nguyễn Văn Bình',birth_date:'2001-01-01',age:25,phone:'',gender:'Nam'}],
  '/api/departments':[{name:'Khoa Cap cuu',accepting_checkins:true,message:'Hoạt động 24/7'}],
  '/api/checkins':[{checkin_id:10,patient_id:1,department:'Khoa Cap cuu',priority:1,checkin_time:'2026-10-04 10:00:00'}],
  '/api/queue':[{checkin_id:10,patient_id:1,department:'Khoa Cap cuu',current_priority:1,checkin_time:'2026-10-04 10:00:00'}],
  '/api/assignments':[{checkin_id:10,patient_id:1,department:'Khoa Cap cuu',doctor_name:'Bác sĩ An',doctor_id:'BS001',start_time:'2026-10-04 10:00:00',planned_end_time:'2026-10-04 10:20:00',duration_minutes:20}],
  '/api/exams?active=false':[{checkin_id:10,patient_id:1,department:'Khoa Cap cuu',doctor_name:'Bác sĩ An',doctor_id:'BS001',start_time:'2026-10-04 10:00:00',end_time:null,diagnosis:malicious}],
  '/api/doctors':[{id:'BS001',name:'Bác sĩ An',department:'Khoa Cap cuu',experience_years:10,status:'on_duty',duty_mode:'auto',busy:false}]
};
const document = {querySelector:element,activeElement:null,body:element('body'),listeners:{},addEventListener(type,handler){this.listeners[type]=handler;}};
const context = vm.createContext({document,location:{hash:''},window:{addEventListener(){}},console,Map,Set,Date,
  setInterval(handler,ms){ assert.equal(ms,5000); context.refreshTimer=handler; },
  FormData: class {constructor(form){this.data=form.values;}[Symbol.iterator](){return Object.entries(this.data)[Symbol.iterator]();}},
  fetch:async (url,options) => {calls.push([url,options]);return {ok:true,json:async()=>({ok:true,data:fixture[url] ?? {assigned_count:1}})};}});
vm.runInContext(readFileSync(resolve(__dirname,'../web/app.js'),'utf8'),context);
const run = code => vm.runInContext(code,context);

(async()=>{
  await new Promise(resolve=>setImmediate(resolve));
  assert(run('state.loaded'));
  assert(calls.every(([,options])=>options.method==='GET'),'Opening page must not write');
  for (const view of ['overview','patients','queue','assignments','exams','doctors']) {
    run(`state.view=${JSON.stringify(view)};render()`);
    assert(element('#content').innerHTML.length > 100);
    assert(!element('#content').innerHTML.includes(malicious),'Data must be escaped');
  }
  assert(run("normalize('Nguyễn Văn Đạt')") === 'nguyen van dat');
  run("state.view='patients';render()");
  assert(!element('#content').innerHTML.includes('&lt;img'),'Checked-in patient leaves intake list');
  run("state.view='queue';render()");
  assert(element('#content').innerHTML.includes('&lt;img'),'Checked-in patient appears in queue');
  assert(!element('#content').innerHTML.includes('data-action="sync"'));
  await context.refreshTimer();
  assert(!run('state.busy'));
  run("state.view='patients';state.search='nguyen';render()");
  assert(element('#content').innerHTML.includes('Nguyễn Văn Bình'));
  assert(!element('#content').innerHTML.includes('&lt;img'));
  run("state.search='';openModal('new-patient')");
  assert(element('#modal-body').innerHTML.includes('birth_date'));
  run("openModal('checkin',2)");
  assert(element('#modal-body').innerHTML.includes('value="2" selected'));
  assert(!element('#modal-body').innerHTML.includes('>#1 ·'));
  run("openModal('priority',10)");
  assert(element('#modal-body').innerHTML.includes('value="1" selected'));
  const beforeTimer = calls.length;
  await context.refreshTimer();
  assert.equal(calls.length,beforeTimer,'Background refresh must preserve open form');
  run("openModal('schedule')");
  assert(element('#modal-body').innerHTML.includes('Phân một khoa'));
  assert(element('#modal-body').innerHTML.includes('Phân theo bác sĩ'));
  const form = element('#modal-form');
  form.values={schedule_mode:'doctor',doctor_id:'BS001',department:'ignored'};
  await form.listeners.submit({preventDefault(){},currentTarget:form});
  const doctorAssignment=calls.find(([url,options])=>url==='/api/assignments' && options.method==='POST');
  assert.deepEqual(JSON.parse(doctorAssignment[1].body),{doctor_id:'BS001'});
  run("openModal('schedule')");
  document.listeners.change({target:{id:'schedule-mode',value:'doctor'}});
  assert(element('#schedule-department-field').hidden);
  assert(!element('#schedule-doctor-field').hidden);
  form.values={schedule_mode:'doctor',doctor_id:''};
  const beforeInvalidSchedule=calls.length;
  await form.listeners.submit({preventDefault(){},currentTarget:form});
  assert.equal(calls.length,beforeInvalidSchedule,'Empty doctor selection must not schedule all departments');
  assert(!element('#form-error').hidden);
  document.listeners.change({target:{id:'schedule-mode',value:'department'}});
  assert(!element('#schedule-department-field').hidden);
  form.values={schedule_mode:'department',department:'Khoa Cap cuu'};
  await form.listeners.submit({preventDefault(){},currentTarget:form});
  const departmentAssignment=calls.findLast(([url,options])=>url==='/api/assignments' && options.method==='POST');
  assert.deepEqual(JSON.parse(departmentAssignment[1].body),{department:'Khoa Cap cuu'});
  run("openModal('doctor-busy','BS001')");
  form.values={busy_minutes:'30',busy_reason:'Họp gấp'};
  await form.listeners.submit({preventDefault(){},currentTarget:form});
  const doctorWrite=calls.find(([url,options])=>url==='/api/doctors/BS001/status' && options.method==='PATCH');
  assert.deepEqual(JSON.parse(doctorWrite[1].body),{busy_minutes:30,busy_reason:'Họp gấp'});
  run("openModal('diagnosis',10)");
  assert(element('#modal-body').innerHTML.includes('&lt;img'));
  run("openModal('exam-detail',10)");
  assert(element('#modal-submit').hidden);
  run("openModal('new-patient')");
  form.values = {name:' Test ',birth_date:'2000-01-01',gender:'Nam',phone:'',hometown:'',address:'',height:'170',weight:'65'};
  await form.listeners.submit({preventDefault(){},currentTarget:form});
  const post = calls.find(([url,options])=>url==='/api/patients' && options.method==='POST');
  assert(post);
  assert.deepEqual(JSON.parse(post[1].body),{name:'Test',birth_date:'2000-01-01',gender:'Nam',phone:'',hometown:'',address:'',height:170,weight:65});
  assert(!element('#modal').open);
  // A successful write followed by a failed refresh must be reported as already saved.
  context.fetch = async()=>{throw Error('offline');};
  await run("mutate(async()=>({}), 'Đã lưu', false)");
  assert(element('#notice').innerHTML.includes('đã được lưu'));
  assert(!run('state.busy'));
  console.log('PASS: UI views, auto refresh/form preservation, intake-to-queue, both scheduling modes, doctor busy, validation, escaping, saved-write refresh failure');
})().catch(error=>{console.error(error);process.exitCode=1;});
