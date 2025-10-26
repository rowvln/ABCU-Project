// ProjectTwo.cpp
// ABCU Advising Assistance Program
// 
// Author: Rowvin Dizon
// Date: October 17 2025
//
// -----------------------------------------------------------------------------
// Single-file implementation using a hash table for fast lookups.
// Primary Functions:
//   1. Load course data from a CSV file into a hash table.
//   2. Print all courses in sorted order.
//   3. Print a specific course and its prerequisites.
// Design follows my Project One recommendation of hash table + on-demand sort.
// -----------------------------------------------------------------------------

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
using namespace std;

// -----------------------------------------------------------------------------
// Data Model
// -----------------------------------------------------------------------------
struct Course {
    string number;                 // normalized course code (e.g. "CSCI200")
    string title;                  // descriptive course name
    vector<string> prereqNumbers;  // list of prerequisite course IDs
};

// hash table lookup for O(1) avg insert/search
using CourseTable = unordered_map<string, Course>;

struct ProgramState {
    bool loaded = false;
    CourseTable courses;
    vector<string> sortedKeys; // cached for consistent alphanumeric output
};

// -----------------------------------------------------------------------------
// Utility Helpers
// -----------------------------------------------------------------------------

// trim helpers
static inline string ltrim(string s) {
    size_t i = 0;
    while (i < s.size() && isspace((unsigned char)s[i])) ++i;
    return s.substr(i);
}
static inline string rtrim(string s) {
    if (s.empty()) return s;
    size_t i = s.size() - 1;
    while (i < s.size() && isspace((unsigned char)s[i])) {
        if (i == 0) return "";
        --i;
    }
    return s.substr(0, i + 1);
}
static inline string trim(string s) { return rtrim(ltrim(std::move(s))); }

// Normalize course IDs → uppercase, strip spaces/dashes/underscores.
// Accepts input like "cs-200" or "  cs 200 ".
static string normalizeCourseId(string s) {
    string out;
    for (char ch : s) {
        if (isspace((unsigned char)ch) || ch == '-' || ch == '_' || ch == ',') continue;
        out.push_back((char)toupper((unsigned char)ch));
    }
    return trim(out);
}

// Minimal quote-aware CSV parser. Handles titles with commas like:
// "CSCI200","Data Structures, with Labs",CSCI100
static vector<string> splitCSV(const string& line) {
    vector<string> out;
    string cur;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == '"') {
            // handle escaped quotes
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                cur.push_back('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            out.push_back(trim(cur));
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    out.push_back(trim(cur));
    return out;
}

static void printDivider() { cout << "----------------------------------------\n"; }

// -----------------------------------------------------------------------------
// Option 1: Load File Data
// -----------------------------------------------------------------------------
static bool loadCoursesFromFile(const string& filename, ProgramState& state) {
    ifstream in(filename);
    if (!in) {
        cerr << "Error: could not open \"" << filename << "\".\n";
        return false;
    }

    CourseTable newTable;
    string line;
    size_t lineNum = 0;

    while (getline(in, line)) {
        ++lineNum;
        line = trim(line);
        if (line.empty()) continue;

        auto fields = splitCSV(line);
        if (fields.size() < 2) {
            cerr << "Warning: malformed line " << lineNum << ".\n";
            continue;
        }

        Course c;
        c.number = normalizeCourseId(fields[0]);
        c.title  = fields[1];

        for (size_t i = 2; i < fields.size(); ++i) {
            string p = normalizeCourseId(fields[i]);
            if (!p.empty()) c.prereqNumbers.push_back(p);
        }

        if (!c.number.empty()) newTable[c.number] = std::move(c);
    }

    // Replace the program state only after the entire file has been parsed successfully
    state.courses.swap(newTable);

    // Store pre-sorted course numbers to avoid re-sorting each time the list is printed
    state.sortedKeys.clear();
    state.sortedKeys.reserve(state.courses.size());
    for (auto& kv : state.courses) state.sortedKeys.push_back(kv.first);
    sort(state.sortedKeys.begin(), state.sortedKeys.end());
    state.loaded = true;

    cout << "Loaded " << state.courses.size() << " courses from \"" << filename << "\".\n";
    return true;
}

// -----------------------------------------------------------------------------
// Option 2: Print full course list (alphanumeric)
// -----------------------------------------------------------------------------
static void printCourseList(const ProgramState& state) {
    if (!state.loaded) {
        cout << "Please load the data first (Option 1).\n";
        return;
    }
    for (const string& id : state.sortedKeys)
        cout << id << ", " << state.courses.at(id).title << '\n';
}

// -----------------------------------------------------------------------------
// Option 3: Print single course + prerequisites
// -----------------------------------------------------------------------------
static void printSingleCourse(const ProgramState& state) {
    if (!state.loaded) {
        cout << "Please load the data first (Option 1).\n";
        return;
    }

    cout << "What course do you want to know about? ";
    string query;
    getline(cin, query);
    query = normalizeCourseId(query);

    if (query.empty()) {
        cout << "No course entered.\n";
        return;
    }

    auto it = state.courses.find(query);
    if (it == state.courses.end()) {
        cout << "Course not found.\n";
        return;
    }

    const Course& c = it->second;
    cout << c.number << ", " << c.title << '\n';

    if (c.prereqNumbers.empty()) {
        cout << "Prerequisites: None\n";
        return;
    }

    cout << "Prerequisites: ";
    for (size_t i = 0; i < c.prereqNumbers.size(); ++i) {
        const string& pid = c.prereqNumbers[i];
        auto p = state.courses.find(pid);
        if (p != state.courses.end())
            cout << p->second.number << " (" << p->second.title << ")";
        else
            cout << pid << " (missing)";
        if (i + 1 < c.prereqNumbers.size()) cout << ", ";
    }
    cout << '\n';
}

// -----------------------------------------------------------------------------
// Menu/Main loop
// -----------------------------------------------------------------------------
static void showMenu() {
    printDivider();
    cout << "1. Load Data Structure\n"
         << "2. Print Course List\n"
         << "3. Print Course\n"
         << "9. Exit\n";
    printDivider();
    cout << "Enter choice: ";
}

int main() {
    cout << "Welcome to the course planner.\n";
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ProgramState state;

    while (true) {
        showMenu();
        string line;
        if (!getline(cin, line)) break;

        int choice = -1;
        try { choice = stoi(trim(line)); } catch (...) {}

        if (choice == 1) {
            cout << "Enter the file name: ";
            string fname;
            getline(cin, fname);
            if (!trim(fname).empty())
                loadCoursesFromFile(trim(fname), state);
            else
                cout << "No file name entered.\n";
        }
        else if (choice == 2) printCourseList(state);
        else if (choice == 3) printSingleCourse(state);
        else if (choice == 9) {
            cout << "Thank you for using the Advising Assistance Program.\n";
            break;
        }
        else
            cout << "That is not a valid option. Try again.\n";
    }
    return 0;
}
