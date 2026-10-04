'use strict';

const $ = selector => document.querySelector(selector);
const escapeHtml = value => String(value ?? '').replace(/[&<>"']/g, char => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[char]));
const normalize = value => String(value ?? '').toLocaleLowerCase('vi').normalize('NFD').replace(/[\u0300-\u036f]/g, '').replace(/đ/g, 'd');
const number = value => Number(value).toLocaleString('vi-VN');
const priorityNames = ['', 'Cấp cứu', 'Rất cao', 'Cao', 'Bình thường', 'Thấp'];
const views = {
  overview: ['Tổng quan', 'Theo dõi hoạt động tiếp nhận và khám bệnh trong cùng một nơi.', 'dashboard'],
  patients: ['Bệnh nhân', 'Quản lý hồ sơ và tiếp nhận bệnh nhân đến khám.', 'people'],
  queue: ['Hàng đợi', 'Theo dõi thứ tự khám và điều chỉnh mức độ ưu tiên.', 'queue'],
  assignments: ['Lịch khám', 'Phân bác sĩ và theo dõi lịch khám đã được sắp xếp.', 'calendar'],
  exams: ['Đang khám', 'Cập nhật chẩn đoán, đơn thuốc và kết thúc lượt khám.', 'medical'],
  doctors: ['Bác sĩ', 'Danh sách bác sĩ và chuyên khoa phụ trách.', 'doctor']
};
const paths = {
  dashboard:'M3 3h7v7H3z M14 3h7v7h-7z M3 14h7v7H3z M14 14h7v7h-7z',
  people:'M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2 M16 3a4 4 0 0 1 0 8 M22 21v-2a4 4 0 0 0-3-3.87 M13 7a4 4 0 1 1-8 0 4 4 0 0 1 8 0',
  queue:'M9 5h12 M9 12h12 M9 19h12 M3 5h1 M3 12h1 M3 19h1',
  calendar:'M8 2v4 M16 2v4 M3 10h18 M5 4h14a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2 M8 14h3 M8 18h6',
  medical:'M9 3h6v6h6v6h-6v6H9v-6H3V9h6z',
  doctor:'M6 3v5a6 6 0 0 0 12 0V3 M6 3H4 M18 3h2 M12 14v3a4 4 0 0 0 8 0v-2 M22 13a2 2 0 1 1-4 0 2 2 0 0 1 4 0',
  search:'M21 21l-5-5 M18 10a7 7 0 1 1-14 0 7 7 0 0 1 14 0',
  refresh:'M20 7v5h-5 M4 17v-5h5 M6 7a7 7 0 0 1 12-2l2 2 M18 17a7 7 0 0 1-12 2l-2-2',
  arrow:'M5 12h14 M14 7l5 5-5 5',
  check:'M5 12l4 4L19 6',
  clock:'M12 8v4l3 2 M22 12a10 10 0 1 1-20 0 10 10 0 0 1 20 0'
};
const icon = name => `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="${paths[name] || paths.medical}"/></svg>`;
const state = { view:'overview', busy:false, loaded:false, modal:null, data:{patients:[],departments:[],checkins:[],queue:[],assignments:[],exams:[],doctors:[]}, search:'', department:'', examMode:'active', page:1 };
const departmentLabel = name => ({'Khoa Cap cuu':'Khoa Cấp cứu','Khoa Noi':'Khoa Nội','Khoa Ngoai':'Khoa Ngoại','Khoa Tim mach':'Khoa Tim mạch','Khoa Nhi':'Khoa Nhi','Khoa San':'Khoa Sản','Khoa Tai Mui Hong':'Khoa Tai Mũi Họng','Khoa Mat':'Khoa Mắt','Khoa Da lieu':'Khoa Da liễu','Khoa Than kinh':'Khoa Thần kinh'}[name] || name);
const initials = name => String(name || '?').trim().split(/\s+/).slice(-2).map(word => word[0]).join('').toUpperCase();
const person = (name, subtitle = '') => `<div class="person"><span class="person-avatar">${escapeHtml(initials(name))}</span><div><strong>${escapeHtml(name || 'Chưa có tên')}</strong><small>${escapeHtml(subtitle)}</small></div></div>`;
const badge = (text, color = '') => `<span class="badge ${color}"><span class="dot"></span>${escapeHtml(text)}</span>`;
const priorityBadge = level => badge(`${level} · ${priorityNames[level] || 'Chưa xác định'}`, ({1:'red',2:'orange',3:'orange',4:'green',5:'gray'}[level] || 'gray'));
const timeLabel = value => {
  if (!value) return '—';
  const [date,time] = value.split(/[ T]/);
  const parts = date.split('-');
  return parts.length === 3 ? `${time?.slice(0,5) || ''} · ${parts[2]}/${parts[1]}/${parts[0]}` : value;
};
const patientName = id => state.patientMap?.get(id)?.name || `Bệnh nhân #${id}`;
const empty = (title, description, symbol = 'queue') => `<div class="empty">${icon(symbol)}<strong>${escapeHtml(title)}</strong><p>${escapeHtml(description)}</p></div>`;
const button = (action, label, id = '', style = 'secondary small') => `<button class="button ${style}" data-action="${action}"${id === '' ? '' : ` data-id="${Number(id)}"`}>${label}</button>`;
const departmentOptions = (selected = '', all = true) => `${all ? '<option value="">Tất cả các khoa</option>' : ''}${state.data.departments.map(d => `<option value="${escapeHtml(d.name)}"${d.name === selected ? ' selected' : ''}>${escapeHtml(departmentLabel(d.name))}</option>`).join('')}`;
const priorityOptions = selected => priorityNames.slice(1).map((name,i) => `<option value="${i+1}"${Number(selected) === i+1 ? ' selected' : ''}>${i+1} · ${name}</option>`).join('');

async function api(url, method = 'GET', data) {
  let response;
  try { response = await fetch(url, {method, headers:{'Content-Type':'application/json'}, ...(data === undefined ? {} : {body:JSON.stringify(data)})}); }
  catch { throw new Error('Không thể kết nối đến server. Hãy kiểm tra server đang chạy rồi thử lại.'); }
  let result;
  try { result = await response.json(); } catch { throw new Error('Server trả về dữ liệu không hợp lệ. Hãy thử làm mới.'); }
  if (!response.ok || !result.ok) throw new Error(result.error || `Thao tác thất bại (${response.status}).`);
  return result.data;
}
function notice(message, kind = 'success') {
  $('#notice').className = `notice ${kind}`;
  $('#notice').innerHTML = `<span>${escapeHtml(message)}</span><button data-action="dismiss" aria-label="Đóng thông báo">×</button>`;
  $('#notice').hidden = false;
}
function setBusy(value) {
  state.busy = value;
  document.body.classList.toggle('busy', value);
  $('#refresh-button').disabled = value;
  $('#modal-submit').disabled = value;
  $('#content').setAttribute('aria-busy', String(value));
}
async function loadData() {
  const endpoints = {patients:'/api/patients', departments:'/api/departments', checkins:'/api/checkins', queue:'/api/queue', assignments:'/api/assignments', exams:'/api/exams?active=false', doctors:'/api/doctors'};
  // Chỉ đọc khi mở trang/làm mới; các thao tác ghi luôn được người dùng bấm rõ ràng.
  const entries = await Promise.all(Object.entries(endpoints).map(async ([name,url]) => [name,await api(url)]));
  state.data = Object.fromEntries(entries);
  state.patientMap = new Map(state.data.patients.map(p => [p.id,p]));
  state.loaded = true;
  $('#connection').className = 'connection';
  $('#connection').innerHTML = '<span class="online-dot"></span> Server đã kết nối';
  $('#last-updated').textContent = `Cập nhật lúc ${new Date().toLocaleTimeString('vi-VN', {hour:'2-digit',minute:'2-digit'})}`;
  render();
}
async function refresh() {
  if (state.busy) return;
  setBusy(true);
  try { await loadData(); }
  catch (error) {
    notice(error.message, 'error');
    $('#connection').className = 'connection offline';
    $('#connection').innerHTML = '<span class="online-dot"></span> Chưa tải được dữ liệu';
    if (!state.loaded) $('#content').innerHTML = empty('Chưa kết nối được bệnh viện', 'Kiểm tra server rồi bấm Làm mới để thử lại.', 'medical');
  } finally { setBusy(false); }
}
function navigation() {
  $('#navigation').innerHTML = Object.entries(views).map(([key,[label,,symbol]]) => `<a class="nav-item ${key === state.view ? 'active' : ''}" href="#${key}"${key === state.view ? ' aria-current="page"' : ''}>${icon(symbol)}<span>${label}</span>${key === 'queue' && state.loaded ? `<span class="nav-count">${number(state.data.queue.length)}</span>` : ''}</a>`).join('');
}
function changeView() {
  const next = location.hash.slice(1) || 'overview';
  if (next !== state.view) { state.search = ''; state.department = ''; state.page = 1; }
  state.view = Object.hasOwn(views,next) ? next : 'overview';
  const [title,description] = views[state.view];
  $('#page-title').textContent = title;
  $('#breadcrumb-current').textContent = title;
  $('#page-description').textContent = description;
  document.title = `${title} · KMIN HEALTH`;
  navigation();
  if (state.loaded) render();
}
function table(headers, rows, emptyTitle = 'Chưa có dữ liệu', description = 'Thông tin sẽ xuất hiện khi có lượt khám mới.') {
  if (!rows.length) return empty(emptyTitle,description);
  return `<div class="table-wrap"><table><thead><tr>${headers.map(h => `<th scope="col">${h}</th>`).join('')}</tr></thead><tbody>${rows.map(row => `<tr>${row.map(cell => `<td>${cell}</td>`).join('')}</tr>`).join('')}</tbody></table></div>`;
}
function filter(records, fields) {
  const query = normalize(state.search);
  return records.filter(r => (!state.department || r.department === state.department || r.doctor_department === state.department) && (!query || fields(r).some(value => normalize(value).includes(query))));
}
function pager(records, row, headers, title, description) {
  const size = 20;
  const pages = Math.max(1, Math.ceil(records.length / size));
  state.page = Math.max(1, Math.min(state.page, pages));
  const offset = (state.page-1)*size;
  return table(headers,records.slice(offset,offset+size).map(row),title,description) + (records.length ? `<div class="pagination"><span>Hiển thị ${offset+1}–${Math.min(offset+size,records.length)} / ${number(records.length)}</span><div><button class="button secondary small" data-action="prev"${state.page === 1 ? ' disabled' : ''}>← Trước</button><span>${state.page} / ${pages}</span><button class="button secondary small" data-action="next"${state.page === pages ? ' disabled' : ''}>Sau →</button></div></div>` : '');
}
function toolbar(placeholder, withDepartment = true, extra = '') {
  return `<div class="toolbar"><div class="filters"><label class="search-box">${icon('search')}<input id="search" type="search" aria-label="Tìm kiếm" placeholder="${placeholder}" value="${escapeHtml(state.search)}"></label>${withDepartment ? `<select id="department-filter" aria-label="Lọc theo khoa">${departmentOptions(state.department)}</select>` : ''}${extra}</div></div>`;
}
function queueRow(r, actions = true) {
  return [person(patientName(r.patient_id),`Mã phiếu #${r.checkin_id}`), escapeHtml(departmentLabel(r.department)), priorityBadge(r.current_priority), escapeHtml(timeLabel(r.checkin_time)), ...(actions ? [`<div class="row-actions">${button('priority','Đổi ưu tiên',r.checkin_id)}</div>`] : [])];
}
function dashboard() {
  const d = state.data;
  const active = d.exams.filter(e => !e.end_time);
  const completed = d.exams.filter(e => e.end_time);
  const stats = [ ['Tổng bệnh nhân',d.patients.length,'Hồ sơ đã được lưu','people',''], ['Đang chờ khám',d.queue.length,'Chưa được phân bác sĩ','clock','amber'], ['Đang khám',active.length,'Lượt khám đang hoạt động','medical','blue'], ['Đã hoàn tất',completed.length,'Lượt khám đã kết thúc','check','purple'] ];
  const latest = [...d.checkins].sort((a,b) => b.checkin_id-a.checkin_id).slice(0,5);
  const stages = [['Tiếp nhận',d.checkins.length,'people'],['Đã phân bác sĩ',d.assignments.length,'calendar'],['Đang khám',active.length,'medical'],['Hoàn tất',completed.length,'check']];
  return `<section class="hero-strip"><div class="hero-copy"><span class="hero-kicker"><span class="online-dot"></span> KMIN HEALTH · HOSPITAL</span><h2>Chăm sóc tận tâm.<br><span>Điều phối thông minh.</span></h2><p>Một không gian kết nối bệnh nhân, bác sĩ và từng lượt khám.<br>Chào mừng bạn đến với trung tâm điều phối KMIN HEALTH.</p><div class="hero-actions"><button class="button primary" data-action="checkin">Tiếp nhận bệnh nhân ${icon('arrow')}</button><a href="#assignments" class="hero-link">Xem lịch khám <span>↗</span></a></div><div class="hero-facts"><span>${number(d.departments.length)} chuyên khoa</span><span>${number(d.doctors.length)} bác sĩ</span></div></div><div class="hero-art" aria-hidden="true"><div class="art-orbit orbit-one"></div><div class="art-orbit orbit-two"></div><div class="art-cross">${icon('medical')}</div><div class="art-tag"><span class="art-tag-icon">${icon('check')}</span><span>KMIN HEALTH<small>Kết nối sức khỏe</small></span></div><svg class="art-pulse" viewBox="0 0 320 80" fill="none"><path d="M0 42h80l14-15 14 28 19-47 21 63 16-29h156" stroke="currentColor" stroke-width="2"/></svg></div></section>
    <div class="section-caption"><div><span class="eyebrow">BỨC TRANH TỔNG QUAN</span><h2>Hoạt động bệnh viện</h2></div><span class="live-label"><span class="online-dot"></span> Dữ liệu tại lần cập nhật gần nhất</span></div>
    <div class="stats-grid">${stats.map(([label,value,note,symbol,color],i) => `<article class="stat-card stat-${i}"><div class="stat-top"><span class="stat-icon ${color}">${icon(symbol)}</span><span class="stat-label">${label}</span></div><div class="stat-value">${number(value)}</div><div class="stat-note"><span class="dot"></span>${note}</div></article>`).join('')}</div>
    <section class="flow-summary" aria-label="Số lượng lượt khám theo trạng thái"><div class="flow-intro">${icon('queue')}<span>Luồng điều phối<small>Tổng số theo trạng thái</small></span></div>${stages.map(([label,count,symbol],i) => `<div class="flow-stage"><span class="flow-symbol">${icon(symbol)}</span><div><strong>${number(count)}</strong><small>${label}</small></div>${i < stages.length-1 ? '<span class="flow-arrow">→</span>' : ''}</div>`).join('')}</section>
    <div class="dashboard-grid"><div><section class="panel"><div class="panel-header"><div><h2>Hàng đợi khám bệnh</h2><p>Sắp theo khoa và mức độ ưu tiên</p></div><a class="text-button" href="#queue">Xem tất cả ${icon('arrow')}</a></div>${table(['BỆNH NHÂN','KHOA','ƯU TIÊN','GIỜ TIẾP NHẬN'],d.queue.slice(0,6).map(r => queueRow(r,false)),'Hàng đợi đang trống','Tiếp nhận bệnh nhân hoặc đồng bộ để cập nhật hàng đợi.')}</section>
    <section class="panel"><div class="panel-header"><div><h2>Tiếp nhận gần đây</h2><p>Các phiếu check-in mới nhất</p></div><span class="badge gray">${number(d.checkins.length)} phiếu</span></div>${table(['BỆNH NHÂN','KHOA','ƯU TIÊN','GIỜ TIẾP NHẬN'],latest.map(r => [person(patientName(r.patient_id),`Mã phiếu #${r.checkin_id}`),escapeHtml(departmentLabel(r.department)),priorityBadge(r.priority),escapeHtml(timeLabel(r.checkin_time))]),'Chưa có lượt tiếp nhận','Bắt đầu bằng cách thêm một bệnh nhân và check-in.','people')}</section></div>
    <div class="dashboard-side"><section class="panel"><div class="panel-header"><div><h2>Thao tác nhanh</h2><p>Điều phối trong một bước</p></div></div><div class="panel-body quick-actions"><button class="quick-action" data-action="new-patient">${icon('people')}Thêm bệnh nhân</button><button class="quick-action" data-action="checkin">${icon('medical')}Tiếp nhận</button><button class="quick-action" data-action="schedule">${icon('calendar')}Phân bác sĩ</button><button class="quick-action" data-action="sync">${icon('refresh')}Đồng bộ lượt khám</button></div></section>
    <section class="panel"><div class="panel-header"><div><h2>Trạng thái các khoa</h2><p>Khả năng tiếp nhận tại thời điểm cập nhật</p></div></div><div class="panel-body">${d.departments.map(dept => `<div class="department-row"><span class="dept-icon">${icon('medical')}</span><div><strong>${escapeHtml(departmentLabel(dept.name))}</strong><small>${dept.name === 'Khoa Cap cuu' ? 'Tiếp nhận 24/7' : '07:30–10:00 · 13:00–15:00'}</small></div>${badge(dept.accepting_checkins ? 'Tiếp nhận' : 'Ngoài giờ',dept.accepting_checkins ? 'green' : 'gray')}</div>`).join('')}</div></section></div></div>`;
}
function renderList() {
  const d = state.data;
  if (state.view === 'patients') {
    const records = filter(d.patients,p => [p.name,p.phone,p.id,p.birth_date]);
    const checked = new Set(d.checkins.map(r => r.patient_id));
    return toolbar('Tìm tên, mã bệnh nhân, số điện thoại…',false) + `<section class="panel"><div class="panel-header"><h2>Hồ sơ bệnh nhân</h2><span class="result-count">${number(records.length)} hồ sơ</span></div>${pager(records,p => [person(p.name,`BN-${String(p.id).padStart(4,'0')}`),escapeHtml(p.birth_date),`${p.age} tuổi`,escapeHtml(p.gender || '—'),escapeHtml(p.phone || '—'),checked.has(p.id) ? badge('Đã tiếp nhận','green') : badge('Chưa tiếp nhận','gray'),`<div class="row-actions">${button('edit-patient','Sửa',p.id)}${checked.has(p.id) ? '' : button('checkin','Check-in',p.id,'primary small')}${checked.has(p.id) ? '' : button('delete-patient','Xóa',p.id,'danger small')}</div>`],['BỆNH NHÂN','NGÀY SINH','TUỔI','GIỚI TÍNH','ĐIỆN THOẠI','TRẠNG THÁI','THAO TÁC'],'Không tìm thấy bệnh nhân','Thêm hồ sơ mới hoặc thử từ khóa khác.')}</section>`;
  }
  if (state.view === 'queue') {
    const records = filter(d.queue,r => [patientName(r.patient_id),r.patient_id,r.checkin_id]);
    return toolbar('Tìm tên hoặc mã phiếu…') + `<section class="panel"><div class="panel-header"><div><h2>Danh sách chờ khám</h2><p>Ưu tiên 1 cao nhất · ưu tiên 5 thấp nhất</p></div><div class="row-actions">${button('sync',`${icon('refresh')} Đồng bộ`,'','secondary')}${button('schedule','Phân bác sĩ','','primary')}</div></div>${pager(records,r => queueRow(r),['BỆNH NHÂN','KHOA','ƯU TIÊN','GIỜ TIẾP NHẬN','THAO TÁC'],'Chưa có bệnh nhân chờ khám','Bấm Đồng bộ sau khi tiếp nhận để cập nhật danh sách.')}</section>`;
  }
  if (state.view === 'assignments') {
    const records = filter(d.assignments,r => [patientName(r.patient_id),r.doctor_name,r.doctor_id,r.checkin_id]);
    return toolbar('Tìm bệnh nhân, bác sĩ, mã phiếu…') + `<section class="panel"><div class="panel-header"><div><h2>Lịch phân bác sĩ</h2><p>Lịch đã lưu được giữ lại khi phân thêm lượt khám</p></div>${button('schedule','Phân bác sĩ','','primary')}</div><div class="schedule-summary">${icon('clock')}Giờ kết thúc dưới đây là giờ dự kiến. Đồng bộ để nhận các ca đã đến giờ khám.</div>${pager(records,r => [person(patientName(r.patient_id),`Mã phiếu #${r.checkin_id}`),escapeHtml(departmentLabel(r.department)),person(r.doctor_name,r.doctor_id),escapeHtml(timeLabel(r.start_time)),`${r.duration_minutes} phút`,escapeHtml(timeLabel(r.planned_end_time)),examStatus(r.checkin_id)],['BỆNH NHÂN','KHOA','BÁC SĨ','BẮT ĐẦU','THỜI LƯỢNG','KẾT THÚC DỰ KIẾN','TRẠNG THÁI'],'Chưa có lịch khám','Bấm Phân bác sĩ để sắp xếp các bệnh nhân đang chờ.')}</section>`;
  }
  if (state.view === 'exams') {
    const records = filter(d.exams.filter(e => state.examMode === 'all' || (state.examMode === 'completed' ? e.end_time : !e.end_time)),r => [patientName(r.patientId ?? r.patient_id),r.doctor_name,r.checkin_id]);
    const mode = `<select id="exam-mode" aria-label="Lọc trạng thái khám"><option value="active"${state.examMode === 'active' ? ' selected' : ''}>Đang khám</option><option value="completed"${state.examMode === 'completed' ? ' selected' : ''}>Đã hoàn tất</option><option value="all"${state.examMode === 'all' ? ' selected' : ''}>Tất cả lượt khám</option></select>`;
    return toolbar('Tìm bệnh nhân, bác sĩ, mã phiếu…',true,mode) + `<section class="panel"><div class="panel-header"><div><h2>Theo dõi lượt khám</h2><p>Ca đã được phân sẽ xuất hiện khi tới giờ và được đồng bộ</p></div>${button('sync',`${icon('refresh')} Đồng bộ`,'','secondary')}</div>${pager(records,r => [person(patientName(r.patient_id),`Mã phiếu #${r.checkin_id}`),escapeHtml(departmentLabel(r.department)),person(r.doctor_name,r.doctor_id),escapeHtml(timeLabel(r.start_time)),badge(r.end_time ? 'Đã hoàn tất' : 'Đang khám',r.end_time ? 'gray' : 'blue'),`<div class="row-actions">${button('exam-detail','Chi tiết',r.checkin_id)}${r.end_time ? '' : button('diagnosis','Chẩn đoán',r.checkin_id,'primary small')}${r.end_time ? '' : button('finish','Kết thúc',r.checkin_id)}</div>`],['BỆNH NHÂN','KHOA','BÁC SĨ','BẮT ĐẦU','TRẠNG THÁI','THAO TÁC'],'Chưa có lượt khám phù hợp','Bấm Đồng bộ hoặc chọn trạng thái khác để xem lịch sử.')}</section>`;
  }
  const records = filter(d.doctors,r => [r.name,r.id,r.department]);
  return toolbar('Tìm tên hoặc mã bác sĩ…') + `<section class="panel"><div class="panel-header"><h2>Đội ngũ bác sĩ</h2><span class="result-count">${number(records.length)} bác sĩ</span></div>${pager(records,r => [person(r.name,r.id),escapeHtml(departmentLabel(r.department)),`${r.experience_years} năm`],['BÁC SĨ','CHUYÊN KHOA','KINH NGHIỆM'],'Không tìm thấy bác sĩ','Thử thay đổi khoa hoặc từ khóa tìm kiếm.')}</section>`;
}
function examStatus(id) {
  const exam = state.data.exams.find(e => e.checkin_id === id);
  if (exam) return badge(exam.end_time ? 'Đã hoàn tất' : 'Đang khám',exam.end_time ? 'gray' : 'blue');
  return badge('Đã phân bác sĩ','green');
}
function render() {
  // Giữ con trỏ khi cập nhật danh sách bằng ô tìm kiếm.
  const focused = document.activeElement?.id;
  const selection = focused === 'search' ? $('#search').selectionStart : null;
  navigation();
  $('#content').innerHTML = state.view === 'overview' ? dashboard() : renderList();
  if (focused === 'search') { $('#search')?.focus(); try { $('#search')?.setSelectionRange(selection,selection); } catch {} }
}

const field = (label,name,value = '',type = 'text',attrs = '',full = false) => `<label class="field${full ? ' full' : ''}">${label}<input name="${name}" type="${type}" value="${escapeHtml(value)}" ${attrs}></label>`;
function openModal(type, id) {
  if (state.busy || !state.loaded) return;
  $('#form-error').hidden = true;
  $('#modal-submit').hidden = false;
  $('#modal-submit').className = 'button primary';
  $('#modal-submit').textContent = 'Lưu thông tin';
  state.modal = {type,id};
  let title = '', html = '';
  if (type === 'new-patient' || type === 'edit-patient') {
    const p = type === 'edit-patient' ? state.patientMap.get(id) : {};
    if (!p) return;
    title = type === 'edit-patient' ? 'Cập nhật hồ sơ bệnh nhân' : 'Thêm bệnh nhân mới';
    const today = new Date();
    const maxDate = `${today.getFullYear()}-${String(today.getMonth()+1).padStart(2,'0')}-${String(today.getDate()).padStart(2,'0')}`;
    html = `<p class="form-info">Nhập thông tin để tạo hồ sơ. Tuổi và BMI được tính tự động.</p><div class="form-grid">${field('Họ và tên *','name',p.name,'text','required maxlength="200"',true)}${field('Ngày sinh *','birth_date',p.birth_date,'date',`required max="${maxDate}"`)}<label class="field">Giới tính<select name="gender"><option value="">Chưa cung cấp</option>${[...new Set(['Nam','Nữ','Khác',p.gender].filter(Boolean))].map(g => `<option${p.gender === g ? ' selected' : ''}>${escapeHtml(g)}</option>`).join('')}</select></label>${field('Số điện thoại','phone',p.phone,'tel','maxlength="30"')}${field('Quê quán','hometown',p.hometown,'text','maxlength="300"')}${field('Địa chỉ','address',p.address,'text','maxlength="500"',true)}${field('Chiều cao (cm)','height',p.height || '','number','min="0" max="300" step="0.1"')}${field('Cân nặng (kg)','weight',p.weight || '','number','min="0" max="1000" step="0.1"')}</div>`;
  } else if (type === 'checkin') {
    const checked = new Set(state.data.checkins.map(r => r.patient_id));
    const available = state.data.patients.filter(p => !checked.has(p.id));
    const candidates = available.slice(0,100);
    const selected = available.find(p => p.id === id);
    if (selected && !candidates.includes(selected)) { candidates.unshift(selected); candidates.pop(); }
    title = 'Tiếp nhận bệnh nhân';
    html = `<p class="form-info">Khoa Cấp cứu tiếp nhận 24/7. Các khoa thường nhận bệnh nhân theo giờ hoạt động.</p>${candidates.length ? `<div class="form-grid"><label class="field full">Tìm hồ sơ<input id="patient-picker-search" type="search" placeholder="Nhập tên, mã hoặc số điện thoại…" autocomplete="off"></label><label class="field full">Bệnh nhân *<select name="patient_id" id="patient-picker" required><option value="">Chọn hồ sơ bệnh nhân</option>${candidates.map(p => `<option value="${p.id}"${p.id === id ? ' selected' : ''}>#${p.id} · ${escapeHtml(p.name)} · ${escapeHtml(p.phone || p.birth_date)}</option>`).join('')}</select><small id="picker-count">${number(candidates.length)} hồ sơ chưa check-in</small></label><label class="field">Khoa khám *<select name="department" id="checkin-department">${departmentOptions('Khoa Cap cuu',false)}</select></label><label class="field">Mức độ ưu tiên *<select name="priority">${priorityOptions(4)}</select></label><p class="form-note field full" id="department-reason"></p><label class="check-field"><input type="checkbox" name="transfer_to_emergency">Chuyển sang Khoa Cấp cứu nếu khoa ngoài giờ (chỉ áp dụng cho ưu tiên 1–2).</label></div>` : empty('Chưa có hồ sơ để tiếp nhận','Thêm bệnh nhân mới trước khi thực hiện check-in.','people')}`;
    $('#modal-submit').textContent = 'Xác nhận tiếp nhận';
    $('#modal-submit').hidden = !candidates.length;
  } else if (type === 'priority') {
    const r = state.data.queue.find(r => r.checkin_id === id);
    if (!r) return;
    title = 'Điều chỉnh mức ưu tiên';
    html = `<p class="form-info">${escapeHtml(patientName(r.patient_id))} · Phiếu #${id}</p><label class="field">Mức độ ưu tiên mới<select name="priority">${priorityOptions(r.current_priority)}</select></label>`;
  } else if (type === 'schedule') {
    title = 'Phân bác sĩ cho hàng đợi';
    html = `<p class="form-info">Hệ thống sắp xếp bác sĩ theo khoa, thứ tự ưu tiên và lịch trống. Các lịch đã lưu được giữ nguyên.</p><label class="field">Phạm vi phân bác sĩ<select name="department">${departmentOptions(state.department)}</select></label><p class="form-note">Thời lượng khám dự kiến được mô phỏng từ 10–30 phút.</p>`;
    $('#modal-submit').textContent = 'Phân bác sĩ';
  } else if (type === 'diagnosis' || type === 'exam-detail') {
    const e = state.data.exams.find(e => e.checkin_id === id);
    if (!e) return;
    title = type === 'diagnosis' ? 'Cập nhật chẩn đoán' : 'Chi tiết lượt khám';
    html = `<dl class="detail-grid"><div><dt>Bệnh nhân</dt><dd>${escapeHtml(patientName(e.patient_id))}</dd></div><div><dt>Bác sĩ</dt><dd>${escapeHtml(e.doctor_name || '—')}</dd></div><div><dt>Khoa khám</dt><dd>${escapeHtml(departmentLabel(e.department))}</dd></div><div><dt>Mã phiếu</dt><dd>#${id}</dd></div><div><dt>Bắt đầu</dt><dd>${escapeHtml(timeLabel(e.start_time))}</dd></div><div><dt>Kết thúc thực tế</dt><dd>${escapeHtml(timeLabel(e.end_time))}</dd></div></dl>`;
    if (type === 'diagnosis') {
      html += `<div class="form-grid">${[['Chẩn đoán *','diagnosis',e.diagnosis,true],['Đơn thuốc','prescription',e.prescription,false],['Lời nhắc của bác sĩ','reminder',e.reminder,false]].map(([label,name,value,required]) => `<label class="field full">${label}<textarea name="${name}" maxlength="10000"${required ? ' required' : ''}>${escapeHtml(value)}</textarea></label>`).join('')}</div>`;
    } else {
      html += [['Chẩn đoán',e.diagnosis],['Đơn thuốc',e.prescription],['Lời nhắc',e.reminder]].map(([label,value]) => `<div class="record-block"><strong>${label}</strong>${escapeHtml(value || 'Chưa cập nhật')}</div>`).join('');
      $('#modal-submit').hidden = true;
    }
  } else if (type === 'delete-patient' || type === 'finish') {
    const p = type === 'delete-patient' ? state.patientMap.get(id) : state.data.exams.find(e => e.checkin_id === id);
    if (!p) return;
    title = type === 'delete-patient' ? 'Xóa hồ sơ bệnh nhân?' : 'Kết thúc lượt khám?';
    html = `<p class="form-info">${escapeHtml(type === 'delete-patient' ? p.name : patientName(p.patient_id))}</p><p class="form-note">${type === 'delete-patient' ? 'Hồ sơ này sẽ được xóa khỏi danh sách. Chỉ có thể xóa bệnh nhân chưa check-in.' : 'Lượt khám sẽ được ghi nhận đã hoàn tất với giờ kết thúc hiện tại. Sau đó không thể sửa chẩn đoán của lượt khám này.'}</p>`;
    $('#modal-submit').textContent = type === 'delete-patient' ? 'Xóa hồ sơ' : 'Xác nhận kết thúc';
    if (type === 'delete-patient') $('#modal-submit').className = 'button danger';
  }
  $('#modal-title').textContent = title;
  $('#modal-body').innerHTML = html;
  if (!$('#modal').open) $('#modal').showModal();
  updateDepartmentReason();
}
function updateDepartmentReason() {
  const select = $('#checkin-department');
  if (!select) return;
  const dept = state.data.departments.find(d => d.name === select.value);
  $('#department-reason').textContent = dept?.message || '';
}
function closeModal() { if (!state.busy) { $('#modal').close(); state.modal = null; } }
async function mutate(action, successMessage, close = true) {
  if (state.busy) return;
  setBusy(true);
  let committed = false;
  try {
    const result = await action();
    committed = true;
    if (close) { $('#modal').close(); state.modal = null; }
    notice(typeof successMessage === 'function' ? successMessage(result) : successMessage);
    await loadData();
  } catch (error) {
    if (committed) notice(`Thao tác đã được lưu, nhưng chưa tải lại được danh sách. ${error.message}`, 'warning');
    else if ($('#modal').open) { $('#form-error').textContent = error.message; $('#form-error').hidden = false; }
    else notice(error.message, 'error');
  } finally { setBusy(false); }
}
$('#modal-form').addEventListener('submit', async event => {
  event.preventDefault();
  if (state.busy || !state.modal) return;
  const {type,id} = state.modal;
  const data = Object.fromEntries(new FormData(event.currentTarget));
  if (type === 'new-patient' || type === 'edit-patient') {
    data.name = data.name.trim();
    // Để trống các số đo không gửi 0 khi cập nhật hồ sơ cũ.
    for (const key of ['height','weight']) if (data[key] === '') delete data[key]; else data[key] = Number(data[key]);
    await mutate(() => api(`/api/patients${type === 'edit-patient' ? '/'+id : ''}`,type === 'edit-patient' ? 'PATCH' : 'POST',data), type === 'edit-patient' ? 'Đã cập nhật hồ sơ bệnh nhân.' : 'Đã tạo hồ sơ. Bạn có thể chọn Check-in để tiếp nhận bệnh nhân.');
  } else if (type === 'checkin') {
    data.patient_id = Number(data.patient_id); data.priority = Number(data.priority); data.transfer_to_emergency = data.transfer_to_emergency === 'on';
    // Check-in là thao tác ghi riêng; nếu đồng bộ lỗi vẫn báo chính xác phiếu đã lưu.
    await mutate(() => api('/api/checkins','POST',data), result => `Đã tiếp nhận bệnh nhân · Phiếu #${result.checkin_id}. Bấm Đồng bộ để cập nhật hàng đợi.`);
  } else if (type === 'priority') {
    await mutate(() => api(`/api/checkins/${id}/priority`,'PATCH',{priority:Number(data.priority)}),'Đã cập nhật mức độ ưu tiên.');
  } else if (type === 'schedule') {
    await mutate(() => api('/api/assignments','POST',{department:data.department}), r => `Đã phân bác sĩ cho ${r.assigned_count} lượt khám. Bấm Đồng bộ để cập nhật các ca đến giờ khám.`);
  } else if (type === 'diagnosis') {
    await mutate(() => api(`/api/exams/${id}/diagnosis`,'PATCH',data),'Đã lưu chẩn đoán và thông tin điều trị.');
  } else if (type === 'delete-patient') {
    await mutate(() => api(`/api/patients/${id}`,'DELETE'),'Đã xóa hồ sơ bệnh nhân.');
  } else if (type === 'finish') {
    await mutate(() => api(`/api/exams/${id}/finish`,'POST'),'Lượt khám đã hoàn tất. Thông tin được lưu trong lịch sử.');
  }
});
document.addEventListener('click', event => {
  const target = event.target.closest('[data-action]');
  if (!target || target.disabled) return;
  const action = target.dataset.action;
  if (action === 'dismiss') { $('#notice').hidden = true; return; }
  if (state.busy) return;
  if (action === 'refresh') { refresh(); return; }
  if (action === 'close-modal') { closeModal(); return; }
  if (action === 'sync') { mutate(() => api('/api/queue/sync','POST'),'Đã đồng bộ hàng đợi và các lượt khám đến giờ.',false); return; }
  if (action === 'prev' || action === 'next') { state.page += action === 'next' ? 1 : -1; render(); return; }
  openModal(action,Number(target.dataset.id) || undefined);
});
document.addEventListener('input', event => {
  if (event.target.id === 'search') { state.search = event.target.value; state.page = 1; render(); }
  if (event.target.id === 'patient-picker-search') {
    const query = normalize(event.target.value);
    const current = $('#patient-picker').value;
    const checked = new Set(state.data.checkins.map(r => r.patient_id));
    const candidates = state.data.patients.filter(p => !checked.has(p.id) && [p.name,p.id,p.phone].some(value => normalize(value).includes(query))).slice(0,100);
    $('#patient-picker').innerHTML = `<option value="">Chọn hồ sơ bệnh nhân</option>${candidates.map(p => `<option value="${p.id}"${String(p.id) === current ? ' selected' : ''}>#${p.id} · ${escapeHtml(p.name)} · ${escapeHtml(p.phone || p.birth_date)}</option>`).join('')}`;
    $('#picker-count').textContent = `${number(candidates.length)} hồ sơ hiển thị · Nhập từ khóa để tìm thêm (tối đa 100 kết quả)`;
  }
});
document.addEventListener('change', event => {
  if (event.target.id === 'department-filter') { state.department = event.target.value; state.page = 1; render(); }
  if (event.target.id === 'exam-mode') { state.examMode = event.target.value; state.page = 1; render(); }
  if (event.target.id === 'checkin-department') updateDepartmentReason();
});
$('#modal').addEventListener('cancel', event => { if (state.busy) event.preventDefault(); else state.modal = null; });
$('#modal').addEventListener('click', event => { if (event.target === $('#modal')) { const rect = $('#modal').getBoundingClientRect(); if (event.clientX < rect.left || event.clientX > rect.right || event.clientY < rect.top || event.clientY > rect.bottom) closeModal(); } });
window.addEventListener('hashchange',changeView);
$('#refresh-button').innerHTML = `${icon('refresh')} Làm mới`;
$('#header-date').textContent = new Date().toLocaleDateString('vi-VN',{day:'2-digit',month:'long',year:'numeric'});
changeView();
refresh();
