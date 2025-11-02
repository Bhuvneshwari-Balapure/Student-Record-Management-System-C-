// student_db.cpp
// Student Record Management System (file-based, OOP, simple auth & per-user log)
// Compile: g++ -std=c++17 student_db.cpp -o student_db.exe

#include <bits/stdc++.h>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

const string STUDENT_FILE = "students.csv";
const string USERS_FILE = "users.txt"; // stores username and encoded password
const string LOG_FOLDER = "user_logs";

//// Simple XOR "encode" for learning/demo (NOT secure for production)
string xor_encode(const string &s, const string &key = "key123")
{
    string out = s;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = out[i] ^ key[i % key.size()];
    // convert to hex for safe file storage
    static const char *hex = "0123456789ABCDEF";
    string hexs;
    for (unsigned char c : out)
    {
        hexs.push_back(hex[c >> 4]);
        hexs.push_back(hex[c & 0x0F]);
    }
    return hexs;
}
string xor_decode_hex(const string &hexs, const string &key = "key123")
{
    // hex -> bytes
    string bytes;
    if (hexs.size() % 2 != 0)
        return "";
    for (size_t i = 0; i < hexs.size(); i += 2)
    {
        string byte = hexs.substr(i, 2);
        unsigned char v = (unsigned char)strtoul(byte.c_str(), nullptr, 16);
        bytes.push_back((char)v);
    }
    // XOR decode
    for (size_t i = 0; i < bytes.size(); ++i)
        bytes[i] = bytes[i] ^ key[i % key.size()];
    return bytes;
}

struct Student
{
    int id;
    string name;
    int age;
    string branch;
    double cgpa;

    string to_csv() const
    {
        // id,name,age,branch,cgpa
        ostringstream ss;
        ss << id << ',' << '"' << name << '"' << ',' << age << ',' << '"' << branch << '"' << ',' << cgpa;
        return ss.str();
    }

    static Student from_csv_line(const string &line)
    {
        Student s{0, "", 0, "", 0.0};
        // parse CSV with simple approach (handles quoted fields)
        vector<string> fields;
        string cur;
        bool inq = false;
        for (size_t i = 0; i < line.size(); ++i)
        {
            char c = line[i];
            if (c == '"')
            {
                inq = !inq;
                continue;
            }
            if (c == ',' && !inq)
            {
                fields.push_back(cur);
                cur.clear();
            }
            else
                cur.push_back(c);
        }
        fields.push_back(cur);
        if (fields.size() >= 5)
        {
            s.id = stoi(fields[0]);
            s.name = fields[1];
            s.age = stoi(fields[2]);
            s.branch = fields[3];
            s.cgpa = stod(fields[4]);
        }
        return s;
    }
};

class StudentDatabase
{
    vector<Student> db;
    int next_id;

    void refresh_next_id()
    {
        int mx = 0;
        for (auto &s : db)
            if (s.id > mx)
                mx = s.id;
        next_id = mx + 1;
    }

public:
    StudentDatabase() { load(); }

    void load()
    {
        db.clear();
        if (!fs::exists(STUDENT_FILE))
        {
            // create empty file
            ofstream f(STUDENT_FILE);
            f << "";
            f.close();
        }
        ifstream f(STUDENT_FILE);
        string line;
        while (getline(f, line))
        {
            if (line.size() == 0)
                continue;
            Student s = Student::from_csv_line(line);
            if (s.id > 0)
                db.push_back(s);
        }
        f.close();
        refresh_next_id();
    }

    void save()
    {
        ofstream f(STUDENT_FILE, ios::trunc);
        for (auto &s : db)
            f << s.to_csv() << '\n';
        f.close();
    }

    // CRUD
    Student add_student(const string &name, int age, const string &branch, double cgpa)
    {
        Student s;
        s.id = next_id++;
        s.name = name;
        s.age = age;
        s.branch = branch;
        s.cgpa = cgpa;
        db.push_back(s);
        save();
        return s;
    }

    vector<Student> list_all() const { return db; }

    Student *find_by_id(int id)
    {
        for (auto &s : db)
            if (s.id == id)
                return &s;
        return nullptr;
    }

    bool remove_by_id(int id)
    {
        auto it = remove_if(db.begin(), db.end(), [&](const Student &s)
                            { return s.id == id; });
        if (it == db.end())
            return false;
        bool removed = (it != db.end());
        db.erase(it, db.end());
        save();
        return removed;
    }

    bool update_student(int id, const string &name, int age, const string &branch, double cgpa)
    {
        Student *p = find_by_id(id);
        if (!p)
            return false;
        p->name = name;
        p->age = age;
        p->branch = branch;
        p->cgpa = cgpa;
        save();
        return true;
    }

    vector<Student> search_by_name(const string &term) const
    {
        vector<Student> res;
        string low = term;
        transform(low.begin(), low.end(), low.begin(), ::tolower);
        for (auto &s : db)
        {
            string name = s.name;
            transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find(low) != string::npos)
                res.push_back(s);
        }
        return res;
    }

    // import/export simple CSV
    bool import_csv(const string &path)
    {
        ifstream f(path);
        if (!f.is_open())
            return false;
        string line;
        while (getline(f, line))
        {
            if (line.size() == 0)
                continue;
            Student s = Student::from_csv_line(line);
            // if id conflict, assign new id
            s.id = next_id++;
            db.push_back(s);
        }
        f.close();
        save();
        return true;
    }
};

//// Authentication & user logging (one file per user)
void ensure_user_folder()
{
    if (!fs::exists(LOG_FOLDER))
        fs::create_directory(LOG_FOLDER);
}

bool user_exists(const string &username)
{
    ifstream f(USERS_FILE);
    string u, enc;
    while (f >> u >> enc)
        if (u == username)
        {
            f.close();
            return true;
        }
    f.close();
    return false;
}

bool register_user(const string &username, const string &password)
{
    if (user_exists(username))
        return false;
    ofstream f(USERS_FILE, ios::app);
    f << username << ' ' << xor_encode(password) << '\n';
    f.close();

    // create user's log file
    ensure_user_folder();
    string logf = LOG_FOLDER + "/" + username + ".txt";
    ofstream log(logf, ios::app);
    log << "== User: " << username << " created ==\n";
    log.close();
    return true;
}

bool check_credentials(const string &username, const string &password)
{
    ifstream f(USERS_FILE);
    string u, enc;
    while (f >> u >> enc)
    {
        if (u == username)
        {
            string dec = xor_decode_hex(enc);
            f.close();
            return dec == password;
        }
    }
    f.close();
    return false;
}

void append_user_log(const string &username, const string &msg)
{
    ensure_user_folder();
    string logf = LOG_FOLDER + "/" + username + ".txt";
    ofstream log(logf, ios::app);
    // add timestamp
    time_t t = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    log << buf << " - " << msg << '\n';
    log.close();
}

//// Console UI helpers
void print_student(const Student &s)
{
    cout << "ID: " << s.id << " | Name: " << s.name << " | Age: " << s.age
         << " | Branch: " << s.branch << " | CGPA: " << s.cgpa << '\n';
}

int get_int(const string &prompt)
{
    while (true)
    {
        cout << prompt;
        string line;
        if (!getline(cin, line))
            return 0;
        try
        {
            return stoi(line);
        }
        catch (...)
        {
            cout << "Enter a valid number.\n";
        }
    }
}
double get_double(const string &prompt)
{
    while (true)
    {
        cout << prompt;
        string line;
        if (!getline(cin, line))
            return 0.0;
        try
        {
            return stod(line);
        }
        catch (...)
        {
            cout << "Enter a valid number.\n";
        }
    }
}

void pause_anykey()
{
    cout << "\nPress Enter to continue...";
    string s;
    getline(cin, s);
}

//// Menus
void admin_menu(StudentDatabase &db, const string &user)
{
    while (true)
    {
        cout << "\n===== Admin Menu (" << user << ") =====\n";
        cout << "1) Add Student\n2) View All\n3) Search by Name\n4) Search by ID\n5) Update Student\n6) Delete Student\n7) Import CSV\n8) Logout\nChoice: ";
        string choice;
        getline(cin, choice);
        if (choice == "1")
        {
            cout << "Enter name: ";
            string name;
            getline(cin, name);
            int age = get_int("Enter age: ");
            cout << "Enter branch: ";
            string branch;
            getline(cin, branch);
            double cgpa = get_double("Enter CGPA: ");
            Student s = db.add_student(name, age, branch, cgpa);
            cout << "Added student with ID " << s.id << '\n';
            append_user_log(user, "Added student ID " + to_string(s.id));
        }
        else if (choice == "2")
        {
            auto all = db.list_all();
            cout << "Total students: " << all.size() << '\n';
            for (auto &s : all)
                print_student(s);
            append_user_log(user, "Viewed all students");
        }
        else if (choice == "3")
        {
            cout << "Enter search term: ";
            string term;
            getline(cin, term);
            auto res = db.search_by_name(term);
            for (auto &s : res)
                print_student(s);
            append_user_log(user, "Searched name: " + term);
        }
        else if (choice == "4")
        {
            int id = get_int("Enter ID: ");
            Student *p = db.find_by_id(id);
            if (p)
                print_student(*p);
            else
                cout << "Not found.\n";
            append_user_log(user, "Searched ID: " + to_string(id));
        }
        else if (choice == "5")
        {
            int id = get_int("Enter ID to update: ");
            Student *p = db.find_by_id(id);
            if (!p)
            {
                cout << "No student with that ID.\n";
                continue;
            }
            cout << "Current: ";
            print_student(*p);
            cout << "New name: ";
            string name;
            getline(cin, name);
            int age = get_int("New age: ");
            cout << "New branch: ";
            string branch;
            getline(cin, branch);
            double cgpa = get_double("New CGPA: ");
            bool ok = db.update_student(id, name, age, branch, cgpa);
            cout << (ok ? "Updated.\n" : "Update failed.\n");
            append_user_log(user, "Updated ID " + to_string(id));
        }
        else if (choice == "6")
        {
            int id = get_int("Enter ID to delete: ");
            bool ok = db.remove_by_id(id);
            cout << (ok ? "Deleted.\n" : "Delete failed.\n");
            append_user_log(user, "Deleted ID " + to_string(id));
        }
        else if (choice == "7")
        {
            cout << "Enter CSV file path to import: ";
            string path;
            getline(cin, path);
            bool ok = db.import_csv(path);
            cout << (ok ? "Import successful.\n" : "Import failed.\n");
            append_user_log(user, "Imported CSV: " + path);
        }
        else if (choice == "8")
        {
            append_user_log(user, "Logged out");
            break;
        }
        else
            cout << "Invalid choice.\n";
        pause_anykey();
    }
}

void user_menu(StudentDatabase &db, const string &user)
{
    while (true)
    {
        cout << "\n===== User Menu (" << user << ") =====\n";
        cout << "1) View All Students\n2) Search by Name\n3) Search by ID\n4) Logout\nChoice: ";
        string choice;
        getline(cin, choice);
        if (choice == "1")
        {
            auto all = db.list_all();
            for (auto &s : all)
                print_student(s);
            append_user_log(user, "Viewed all students");
        }
        else if (choice == "2")
        {
            cout << "Enter search term: ";
            string term;
            getline(cin, term);
            auto res = db.search_by_name(term);
            for (auto &s : res)
                print_student(s);
            append_user_log(user, "Searched name: " + term);
        }
        else if (choice == "3")
        {
            int id = get_int("Enter ID: ");
            Student *p = db.find_by_id(id);
            if (p)
                print_student(*p);
            else
                cout << "Not found.\n";
            append_user_log(user, "Searched ID: " + to_string(id));
        }
        else if (choice == "4")
        {
            append_user_log(user, "Logged out");
            break;
        }
        else
            cout << "Invalid.\n";
        pause_anykey();
    }
}

int main()
{
    // ensure files/dirs exist
    if (!fs::exists(USERS_FILE))
    {
        ofstream f(USERS_FILE);
        // default admin account: admin/admin
        f << "admin " << xor_encode("admin") << '\n';
        f.close();
    }
    ensure_user_folder();

    StudentDatabase db;
    cout << "==== Student Record Management System ====\n";
    while (true)
    {
        cout << "\nMain Menu:\n1) Register\n2) Login\n3) Exit\nChoice: ";
        string choice;
        getline(cin, choice);
        if (choice == "1")
        {
            cout << "Choose username: ";
            string username;
            getline(cin, username);
            if (username.empty())
            {
                cout << "Invalid.\n";
                continue;
            }
            cout << "Choose password: ";
            string pwd;
            getline(cin, pwd);
            bool ok = register_user(username, pwd);
            if (!ok)
                cout << "User exists. Choose different name.\n";
            else
            {
                cout << "Registered. You can login now.\n";
                append_user_log(username, "Registered");
            }
        }
        else if (choice == "2")
        {
            cout << "Username: ";
            string username;
            getline(cin, username);
            cout << "Password: ";
            string pwd;
            getline(cin, pwd);
            bool ok = check_credentials(username, pwd);
            if (!ok)
            {
                cout << "Login failed.\n";
                continue;
            }
            cout << "Login success. Welcome " << username << '\n';
            append_user_log(username, "Logged in");
            // simple role: admin if username == "admin"
            if (username == "admin")
                admin_menu(db, username);
            else
                user_menu(db, username);
        }
        else if (choice == "3")
        {
            cout << "Goodbye.\n";
            break;
        }
        else
            cout << "Invalid choice.\n";
    }
    return 0;
}
