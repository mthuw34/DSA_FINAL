#pragma once
#include <iostream>
#include <vector>
#include "patient.h"
#include <string.h>
using namespace std;

// Một node trong danh sách liên kết đôi
struct Node {
    Patient data;
    Node *prev;
    Node *next;
};

// Danh sách chứa toàn bộ node
struct FixedCapacityList {
    Node* head;
    Node* tail;
    int capacity;           // số sức chứa tối đa trong danh sách
    int count;              // đếm số người hiện đang có trong danh sách vừa mới khám
};

// Khai báo các hàm thao tác trên FixedCapacityList
FixedCapacityList createList (int k);
void deleteList (FixedCapacityList& list);

void insertExaminedPatient (FixedCapacityList& list, const Patient& p);
void printPatient(const FixedCapacityList& list, const Patient& p);
bool getById(const FixedCapacityList& list, const string& id, Patient& result);
vector <Patient> getAll (const FixedCapacityList& list);
