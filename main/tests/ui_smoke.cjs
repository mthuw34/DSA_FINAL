// Kiểm tra logic giao diện bằng DOM giả, không chạm vào database người dùng.
const { readFileSync } = require('node:fs');
const { resolve } = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const elements = new Map();
function element(key) {
  if (!elements.has(key)) elements.set(key, {
    html:'',writes:0,get innerHTML(){return this.html;},set innerHTML(value){this.html=value;this.writes++;},textContent:'',hidden:false,disabled:false,value:'',open:false,
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
  '/api/doctors':[{id:'BS001',name:'Bác sĩ An',department:'Khoa Cap cuu',experience_years:10,status:'on_duty',duty_mode:'auto',busy:false,
    shift_rule:'24h_on_24h_off',shift_period_start:'2026-10-05',shifts:[{date:'2026-10-05',start_time:'2026-10-05 00:00:00',end_time:'2026-10-06 00:00:00',is_current:true}]}]
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
  const contentWrites=element('#content').writes;
  const navWrites=element('#navigation').writes;
  const oldFetch=context.fetch;
  context.fetch=async (...args)=>{
    assert(!run('state.busy'),'Background refresh must not dim/disable UI');
    return oldFetch(...args);
  };
  await context.refreshTimer();
  context.fetch=oldFetch;
  assert.equal(element('#content').writes,contentWrites,'Unchanged content must retain DOM');
  assert.equal(element('#navigation').writes,navWrites,'Navigation must retain DOM');
  fixture['/api/queue'][0].last_update='2026-10-05 00:00:00';
  await context.refreshTimer();
  assert.equal(element('#content').writes,contentWrites,'Hidden timestamp changes must not rebuild table');
  fixture['/api/queue'][0].current_priority=2;
  await context.refreshTimer();
  assert.equal(element('#content').writes,contentWrites+1,'Visible changes must still update');
  fixture['/api/queue'][0].current_priority=1;
  assert(!run('state.busy'));
  // A slow poll must never overwrite the result loaded after a user's write.
  let releasePoll;
  const heldPoll=new Promise(resolve=>{releasePoll=resolve;});
  context.fetch=async (url,options)=>{
    const snapshot=JSON.parse(JSON.stringify(fixture[url]));
    await heldPoll;
    return {ok:true,json:async()=>({ok:true,data:snapshot})};
  };
  const stalePoll=context.refreshTimer();
  context.fetch=oldFetch;
  fixture['/api/queue'][0].current_priority=3;
  await run("mutate(async()=>({}), 'Đã lưu', false)");
  releasePoll();
  await stalePoll;
  assert.equal(run('state.data.queue[0].current_priority'),3,'Stale background snapshot must be discarded');
  fixture['/api/queue'][0].current_priority=1;
  await context.refreshTimer();
  run("state.view='patients';state.search='nguyen';render()");
  assert(element('#content').innerHTML.includes('Nguyễn Văn Bình'));
  assert(!element('#content').innerHTML.includes('&lt;img'));
  run("state.search='';openModal('new-patient')");
  assert(element('#modal-body').innerHTML.includes('birth_date'));
  run("openModal('doctor-shifts','BS001')");
  assert(element('#modal-body').innerHTML.includes('00:00 · 05/10/2026'));
  assert(element('#modal-body').innerHTML.includes('00:00 · 06/10/2026'));
  assert(element('#modal-body').innerHTML.includes('Nghỉ'));
  assert(element('#modal-submit').hidden);
  run("state.data.doctors[0].duty_mode='off_duty';openModal('doctor-shifts','BS001')");
  assert(element('#modal-body').innerHTML.includes('nghỉ thủ công'));
  run("state.data.doctors[0].duty_mode='auto'");
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
