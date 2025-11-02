# Student Record Management System (C++)

A robust console-based Student Record Management application implemented in C++.  
Utilizes object-oriented programming (OOP), file-based persistence, and a simple authentication system.  
Each user has their own activity log file, and administrators can manage student records (CRUD).  

## Features
- User registration and login with encrypted credentials stored in `users.txt`  
- Admin user (default: `admin` / `admin`) for full control over student records  
- Normal users can view and search student records  
- Full CRUD (Create, Read, Update, Delete) operations on student data for admin  
- Student data stored in `students.csv`; each record includes ID, Name, Age, Branch, CGPA  
- User-specific activity logs (`user_logs/<username>.txt`) recording all actions with timestamps  
- Secure (for demo) XOR-based credential encoding  
- CSV import functionality to bulk load student records  

# File Structure
- `student_db.cpp` — Main source file containing program logic  
- `students.csv` — Stores all student records persistently  
- `users.txt` — Stores registered usernames and encoded passwords  
- `user_logs/` directory — Contains per user log files like `admin.txt`, `john.txt`  

## Technologies Used
- C++ (C++17 standard)  
- Standard library: `<fstream>`, `<filesystem>`, `<iostream>`, `<string>`  
- OOP design (classes for `Student` and `StudentDatabase`)  
- File handling for persistence (students CSV and user logs)  
- Simple encryption technique (XOR encoding) for demonstrative security  

## How to Build & Run
### Windows (MSYS2 MinGW64 or any g++ environment)
```sh
g++ -std=c++17 student_db.cpp -o student_db.exe
./student_db.exe
