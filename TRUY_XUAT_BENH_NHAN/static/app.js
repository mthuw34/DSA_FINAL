const departmentOrder = [
  'Khoa Cap cuu',
  'Khoa Noi',
  'Khoa Ngoai',
  'Khoa Tim mach',
  'Khoa Nhi',
  'Khoa San',
  'Khoa Tai Mui Hong',
  'Khoa Mat',
  'Khoa Da lieu',
  'Khoa Than kinh'
];

const queueContainer = document.querySelector('#queues');
const statusMessage = document.querySelector('#status');
const refreshButton = document.querySelector('#refresh');

function createQueueCard(department, patients) {
  const card = document.createElement('article');
  card.className = 'queue-card';

  const heading = document.createElement('div');
  heading.className = 'queue-heading';
  heading.innerHTML = `<h2>${department}</h2><span class="count">${patients.length}</span>`;
  card.appendChild(heading);

  if (patients.length === 0) {
    const emptyMessage = document.createElement('p');
    emptyMessage.className = 'empty';
    emptyMessage.textContent = 'Chua co benh nhan dang cho.';
    card.appendChild(emptyMessage);
    return card;
  }

  const table = document.createElement('table');
  table.innerHTML = `
    <thead>
      <tr><th>Ma check-in</th><th>Ma benh nhan</th></tr>
    </thead>
    <tbody></tbody>
  `;

  const body = table.querySelector('tbody');
  patients.forEach((patient) => {
    const row = document.createElement('tr');
    row.innerHTML = `<td>${patient.ma_check_in}</td><td>${patient.ma_benh_nhan}</td>`;
    body.appendChild(row);
  });

  card.appendChild(table);
  return card;
}

async function loadQueues() {
  refreshButton.disabled = true;
  statusMessage.textContent = 'Dang tai danh sach check-in...';

  try {
    const response = await fetch('/api/queues');
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    const result = await response.json();
    if (result.trang_thai !== 'thanh_cong') {
      throw new Error(result.thong_bao || 'Khong tai duoc du lieu');
    }

    queueContainer.replaceChildren(
      ...departmentOrder.map((department) =>
        createQueueCard(department, result[department] || [])
      )
    );

    const total = departmentOrder.reduce(
      (sum, department) => sum + (result[department] || []).length,
      0
    );
    statusMessage.textContent = `Da tai ${total} benh nhan trong 10 khoa.`;
  } catch (error) {
    queueContainer.replaceChildren();
    statusMessage.textContent = `Khong tai duoc du lieu: ${error.message}`;
  } finally {
    refreshButton.disabled = false;
  }
}

refreshButton.addEventListener('click', loadQueues);
loadQueues();
