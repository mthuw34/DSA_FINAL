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
  '/api/doctors':[{id:'BS001',name:'Bác sĩ An',department:'Khoa Cap cuu',experience_years:10}]
};
const document = {querySelector:element,activeElement:null,body:element('body'),listeners:{},addEventListener(type,handler){this.listeners[type]=handler;}};
const context = vm.createContext({document,location:{hash:''},window:{addEventListener(){}},console,Map,Set,Date,
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
  run("openModal('diagnosis',10)");
  assert(element('#modal-body').innerHTML.includes('&lt;img'));
  run("openModal('exam-detail',10)");
  assert(element('#modal-submit').hidden);
  run("openModal('new-patient')");
  const form = element('#modal-form');
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
  console.log('PASS: UI views, read-only startup, escaping, search, dialogs, submit payload, saved-write refresh failure');
})().catch(error=>{console.error(error);process.exitCode=1;});
