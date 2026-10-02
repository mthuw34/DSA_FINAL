#include "FixedCapacityList.h"

// Tạo danh sách hồ sơ bệnh nhân vừa được khám gần nhất
FixedCapacityList createList (int k)    
{
    FixedCapacityList list;
    list.head = NULL;
    list.tail = NULL;
    list.capacity = k;
    list.count = 0;
    return list;
}
// Xoá danh sách 
void deleteList (FixedCapacityList& list)
{
    Node *cur = list.head;
    while (cur != NULL)
    {
        Node* next = cur -> next;
        delete cur;
        cur = next;
    }
    list.head = NULL;
    list.tail = NULL;
    list.count  = 0;
    return;
}
// Thêm bệnh nhân vừa khám vào đầu danh sách
void insertExaminedPatient (FixedCapacityList& list, const Patient& p)
{
    Node* newNode = new Node;
    newNode -> data = p;
    newNode -> prev = NULL;
    newNode -> next = NULL;
    
    if (list.head == NULL)              // Xử lý nếu list rỗng
    {
        list.head = newNode;
        list.tail = newNode;
    }
    else
    {
        newNode -> next = list.head;
        list.head -> prev = newNode;
        list.head = newNode;
    }

    list.count++;                       // Cập nhật đếm số lượng hồ sơ bệnh nhân trong danh sách

    if  (list.count > list.capacity)    // Xử lý nếu count vượt khỏi capacity
    {
        Node* remove = list.tail;
        list.tail = list.tail -> prev;
        list.tail -> next = NULL;
        delete remove;
        list.count --;
    }
}
// Hàm in ra thông tin về một bệnh nhân trong danh sách
void printPatient(const FixedCapacityList& list, const Patient& p)
{
    cout << "Ten benh nhan: " << p.name << endl;
    cout << "ID: " << p.id << endl;
    cout << "Nam sinh: " << p.birthYear << endl;
    cout << "SDT: " << p.phone << endl;
    cout << "Gioi tinh: " << p.gender << endl;
    cout << "Dia chi: " << p.address << endl;
    cout << "Chan doan: " << p.lastDiagnosis << endl;
    cout << "Thoi gian kham xong: " << p.time.hour << ":" << p.time.minute << ":" << p.time.second << " ngay " << p.time.date << "/" << p.time.month << "/" << p.time.year;
    return;
}

// Hàm phát hiện/ truy xuất bệnh nhân trong danh sách dựa trên ID và lưu vào tham chiếu result
bool getById(const FixedCapacityList& list, const string& id, Patient& result)
{
    Node* cur = list.head;
    while (cur != NULL)
    {
        if (cur ->data.id == id)
        {
            result = cur->data;
            return true;
        }
        cur = cur -> next;
    }
    return false;
}
// Hàm lấy ra toàn bộ danh sách bệnh nhân từ DSLK vào mảng (vector)
vector <Patient> getAll (const FixedCapacityList& list)
{
    vector <Patient> result;
    Node* cur = list.head;
    while (cur != NULL)
    {
        result.push_back(cur->data);
        cur = cur-> next;
    }
    return result;
}


