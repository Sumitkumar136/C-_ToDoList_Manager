#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <limits>
#include <algorithm>

using namespace std;

const string USER_FILE = "user_details.txt";
const string TASKS_DIR = "tasks/";
const int MAX_LOGIN_ATTEMPTS = 3;
const int MAX_DESCRIPTION_LENGTH = 200;

// Priority levels for tasks
enum Priority {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3
};

// Task status
enum TaskStatus {
    PENDING,
    IN_PROGRESS,
    COMPLETED
};

class Task {
public:
    string description;
    string dueDate;
    Priority priority;
    TaskStatus status;
    time_t createdDate;
    time_t completedDate;

    Task() {
        description = "";
        dueDate = "";
        priority = MEDIUM;
        status = PENDING;
        createdDate = time(0);
        completedDate = 0;
    }

    Task(const string& desc, const string& date, Priority prio) {
        description = desc;
        dueDate = date;
        priority = prio;
        status = PENDING;
        createdDate = time(0);
        completedDate = 0;
    }

    string getPriorityString() const {
        switch (priority) {
            case LOW: return "Low";
            case MEDIUM: return "Medium";
            case HIGH: return "High";
            default: return "Unknown";
        }
    }

    string getStatusString() const {
        switch (status) {
            case PENDING: return "Pending";
            case IN_PROGRESS: return "In Progress";
            case COMPLETED: return "Completed";
            default: return "Unknown";
        }
    }

    string getFormattedDate(time_t timestamp) const {
        struct tm* timeinfo = localtime(&timestamp);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
        return string(buffer);
    }
};

class TaskManager {
private:
    vector<Task> tasks;
    string userID;
    bool isLoggedIn;

    // Simple password hashing function
    string hashPassword(const string& password) {
        unsigned long hash = 5381;
        for (char c : password) {
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        }
        stringstream ss;
        ss << hex << hash;
        return ss.str();
    }

    // Save tasks to file
    void save_tasks() {
        #ifdef _WIN32
        system(("mkdir " + TASKS_DIR + " 2>nul").c_str());
        #else
        system(("mkdir -p " + TASKS_DIR).c_str());
        #endif

        ofstream outFile(TASKS_DIR + userID + ".txt");
        if (!outFile.is_open()) {
            cerr << "Error: Unable to save tasks." << endl;
            return;
        }

        for (const auto& task : tasks) {
            outFile << task.description << "|"
                    << task.dueDate << "|"
                    << static_cast<int>(task.priority) << "|"
                    << static_cast<int>(task.status) << "|"
                    << task.createdDate << "|"
                    << task.completedDate << endl;
        }
        outFile.close();
    }

    // Load tasks from file
    void load_tasks() {
        ifstream inFile(TASKS_DIR + userID + ".txt");
        if (!inFile.is_open()) {
            return;
        }

        tasks.clear();
        string line;
        while (getline(inFile, line)) {
            vector<string> parts;
            stringstream ss(line);
            string part;
            while (getline(ss, part, '|')) {
                parts.push_back(part);
            }

            if (parts.size() >= 6) {
                Task task;
                task.description = parts[0];
                task.dueDate = parts[1];
                task.priority = static_cast<Priority>(stoi(parts[2]));
                task.status = static_cast<TaskStatus>(stoi(parts[3]));
                task.createdDate = stoll(parts[4]);
                task.completedDate = stoll(parts[5]);
                tasks.push_back(task);
            }
        }
        inFile.close();
    }

    // Validate date format (YYYY-MM-DD)
    bool isValidDate(const string& date) {
        if (date.length() != 10) return false;
        if (date[4] != '-' || date[7] != '-') return false;

        for (int i = 0; i < 10; i++) {
            if (i == 4 || i == 7) continue;
            if (!isdigit(date[i])) return false;
        }

        int year = stoi(date.substr(0, 4));
        int month = stoi(date.substr(5, 2));
        int day = stoi(date.substr(8, 2));

        if (year < 2023 || year > 2100) return false;
        if (month < 1 || month > 12) return false;
        if (day < 1 || day > 31) return false;

        if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) return false;
        if (month == 2) {
            bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            if (day > (isLeap ? 29 : 28)) return false;
        }

        return true;
    }

    // Get current date in YYYY-MM-DD format
    string getCurrentDate() const {
        time_t now = time(0);
        struct tm* timeinfo = localtime(&now);
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
        return string(buffer);
    }

public:
    TaskManager() {
        isLoggedIn = false;
        userID = "";
    }

    // Create a new user account
    bool create_account() {
        string password, confirmPassword;

        cout << "\n=== Create Your Account ===" << endl;

        while (true) {
            cout << "Enter User ID (alphanumeric, 3-20 characters): ";
            getline(cin, userID);

            if (userID.length() < 3 || userID.length() > 20) {
                cout << "User ID must be between 3 and 20 characters." << endl;
                continue;
            }

            bool valid = true;
            for (char c : userID) {
                if (!isalnum(static_cast<unsigned char>(c))) {
                    valid = false;
                    break;
                }
            }

            if (!valid) {
                cout << "User ID must contain only alphanumeric characters." << endl;
                continue;
            }

            ifstream userFile(USER_FILE);
            string stored_userID;
            bool exists = false;

            if (userFile.is_open()) {
                while (userFile >> stored_userID) {
                    userFile.ignore(numeric_limits<streamsize>::max(), '\n'); // Skip password hash
                    if (userID == stored_userID) {
                        exists = true;
                        break;
                    }
                }
                userFile.close();
            }

            if (exists) {
                cout << "User ID already exists. Please choose another one." << endl;
                continue;
            }

            break;
        }

        while (true) {
            cout << "Enter Password (at least 8 characters): ";
            getline(cin, password);

            if (password.length() < 8) {
                cout << "Password must be at least 8 characters long." << endl;
                continue;
            }

            cout << "Confirm Password: ";
            getline(cin, confirmPassword);

            if (password != confirmPassword) {
                cout << "Passwords do not match. Please try again." << endl;
                continue;
            }

            break;
        }

        string hashedPassword = hashPassword(password);

        ofstream userFile(USER_FILE, ios::app);
        if (userFile.is_open()) {
            userFile << userID << " " << hashedPassword << endl;
            userFile.close();
            cout << "Account created successfully!" << endl;
            isLoggedIn = true;
            // start with empty tasks
            tasks.clear();
            save_tasks();
            return true;
        } else {
            cout << "Error: Unable to create account." << endl;
            return false;
        }
    }

    // Log in to existing account
    bool login() {
        string password;
        int attempts = 0;

        cout << "\n=== Log In to Your Account ===" << endl;

        while (attempts < MAX_LOGIN_ATTEMPTS) {
            cout << "Enter User ID: ";
            getline(cin, userID);
            cout << "Enter Password: ";
            getline(cin, password);

            ifstream userFile(USER_FILE);
            if (userFile.is_open()) {
                string stored_userID, stored_password;
                bool found = false;

                while (userFile >> stored_userID >> stored_password) {
                    if (userID == stored_userID && hashPassword(password) == stored_password) {
                        found = true;
                        break;
                    }
                }
                userFile.close();

                if (found) {
                    cout << "Login successful!" << endl;
                    isLoggedIn = true;
                    load_tasks();
                    return true;
                } else {
                    attempts++;
                    cout << "Invalid User ID or Password. ";
                    cout << "Attempts remaining: " << (MAX_LOGIN_ATTEMPTS - attempts) << endl;
                }
            } else {
                cout << "Error: Unable to access user database." << endl;
                return false;
            }
        }

        cout << "Maximum login attempts reached. Please try again later." << endl;
        return false;
    }

    // Add a new task
    void add_task() {
        string description, dueDate;
        int priorityChoice;
        Priority priority;

        cout << "\n=== Add New Task ===" << endl;

        while (true) {
            cout << "Enter task description (max " << MAX_DESCRIPTION_LENGTH << " characters): ";
            getline(cin, description);

            if (description.empty()) {
                cout << "Task description cannot be empty." << endl;
                continue;
            }

            if (description.length() > MAX_DESCRIPTION_LENGTH) {
                cout << "Task description too long. Maximum " << MAX_DESCRIPTION_LENGTH << " characters allowed." << endl;
                continue;
            }

            break;
        }

        while (true) {
            cout << "Enter due date (YYYY-MM-DD, leave empty for no due date): ";
            getline(cin, dueDate);

            if (dueDate.empty()) {
                break; // No due date is allowed
            }

            if (!isValidDate(dueDate)) {
                cout << "Invalid date format. Please use YYYY-MM-DD format." << endl;
                continue;
            }

            string currentDate = getCurrentDate();
            if (dueDate < currentDate) {
                cout << "Due date must be today or in the future." << endl;
                continue;
            }

            break;
        }

        while (true) {
            cout << "Select priority:" << endl;
            cout << "1. Low" << endl;
            cout << "2. Medium" << endl;
            cout << "3. High" << endl;
            cout << "Enter choice (1-3): ";
            if (!(cin >> priorityChoice)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a number." << endl;
                continue;
            }
            cin.ignore();

            if (priorityChoice < 1 || priorityChoice > 3) {
                cout << "Invalid choice. Please enter a number between 1 and 3." << endl;
                continue;
            }

            priority = static_cast<Priority>(priorityChoice);
            break;
        }

        Task newTask(description, dueDate, priority);
        tasks.push_back(newTask);

        cout << "Task added successfully!" << endl;
        save_tasks();
    }

    // View all tasks
    void view_tasks() const {
        cout << "\n=== Your Tasks ===" << endl;

        if (tasks.empty()) {
            cout << "No tasks available." << endl;
            return;
        }

        cout << left << setw(5) << "ID"
             << setw(30) << "Description"
             << setw(12) << "Due Date"
             << setw(10) << "Priority"
             << setw(15) << "Status"
             << setw(12) << "Created" << endl;
        cout << string(84, '-') << endl;

        for (size_t i = 0; i < tasks.size(); ++i) {
            cout << left << setw(5) << (i + 1)
                 << setw(30) << (tasks[i].description.length() > 27 ?
                                 tasks[i].description.substr(0, 27) + "..." :
                                 tasks[i].description)
                 << setw(12) << (tasks[i].dueDate.empty() ? "None" : tasks[i].dueDate)
                 << setw(10) << tasks[i].getPriorityString()
                 << setw(15) << tasks[i].getStatusString()
                 << setw(12) << tasks[i].getFormattedDate(tasks[i].createdDate) << endl;
        }

        cout << endl;
    }

    // View task details
    void view_task_details() const {
        if (tasks.empty()) {
            cout << "No tasks available." << endl;
            return;
        }

        int taskIndex;
        cout << "Enter task ID to view details: ";
        if (!(cin >> taskIndex)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input." << endl;
            return;
        }
        cin.ignore();

        if (taskIndex < 1 || taskIndex > static_cast<int>(tasks.size())) {
            cout << "Invalid task ID." << endl;
            return;
        }

        const Task& task = tasks[taskIndex - 1];

        cout << "\n=== Task Details ===" << endl;
        cout << "ID: " << taskIndex << endl;
        cout << "Description: " << task.description << endl;
        cout << "Due Date: " << (task.dueDate.empty() ? "None" : task.dueDate) << endl;
        cout << "Priority: " << task.getPriorityString() << endl;
        cout << "Status: " << task.getStatusString() << endl;
        cout << "Created: " << task.getFormattedDate(task.createdDate) << endl;

        if (task.status == COMPLETED) {
            cout << "Completed: " << task.getFormattedDate(task.completedDate) << endl;
        }

        cout << endl;
    }

    // Update task status
    void update_task_status() {
        if (tasks.empty()) {
            cout << "No tasks available." << endl;
            return;
        }

        int taskIndex, statusChoice;

        view_tasks();
        cout << "Enter task ID to update status: ";
        if (!(cin >> taskIndex)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input." << endl;
            return;
        }
        cin.ignore();

        if (taskIndex < 1 || taskIndex > static_cast<int>(tasks.size())) {
            cout << "Invalid task ID." << endl;
            return;
        }

        cout << "Select new status:" << endl;
        cout << "1. Pending" << endl;
        cout << "2. In Progress" << endl;
        cout << "3. Completed" << endl;
        cout << "Enter choice (1-3): ";
        if (!(cin >> statusChoice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input." << endl;
            return;
        }
        cin.ignore();

        if (statusChoice < 1 || statusChoice > 3) {
            cout << "Invalid choice." << endl;
            return;
        }

        TaskStatus newStatus = static_cast<TaskStatus>(statusChoice - 1);
        TaskStatus oldStatus = tasks[taskIndex - 1].status;

        tasks[taskIndex - 1].status = newStatus;

        if (newStatus == COMPLETED && oldStatus != COMPLETED) {
            tasks[taskIndex - 1].completedDate = time(0);
            cout << "Task marked as completed on "
                 << tasks[taskIndex - 1].getFormattedDate(tasks[taskIndex - 1].completedDate) << endl;
        } else {
            cout << "Task status updated to " << tasks[taskIndex - 1].getStatusString() << endl;
        }

        save_tasks();
    }

    // Edit task
    void edit_task() {
        if (tasks.empty()) {
            cout << "No tasks available." << endl;
            return;
        }

        int taskIndex;
        view_tasks();
        cout << "Enter task ID to edit: ";
        if (!(cin >> taskIndex)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input." << endl;
            return;
        }
        cin.ignore();

        if (taskIndex < 1 || taskIndex > static_cast<int>(tasks.size())) {
            cout << "Invalid task ID." << endl;
            return;
        }

        Task& task = tasks[taskIndex - 1];
        string input;
        int choice;

        cout << "\n=== Edit Task ===" << endl;
        cout << "Current details:" << endl;
        cout << "1. Description: " << task.description << endl;
        cout << "2. Due Date: " << (task.dueDate.empty() ? "None" : task.dueDate) << endl;
        cout << "3. Priority: " << task.getPriorityString() << endl;
        cout << "4. Status: " << task.getStatusString() << endl;

        cout << "Enter field number to edit (1-4), or 0 to cancel: ";
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input." << endl;
            return;
        }
        cin.ignore();

        switch (choice) {
            case 0:
                cout << "Edit cancelled." << endl;
                break;

            case 1: // Description
                cout << "Enter new description: ";
                getline(cin, input);

                if (input.empty()) {
                    cout << "Description cannot be empty." << endl;
                    break;
                }

                if (input.length() > MAX_DESCRIPTION_LENGTH) {
                    cout << "Description too long. Maximum " << MAX_DESCRIPTION_LENGTH << " characters allowed." << endl;
                    break;
                }

                task.description = input;
                cout << "Description updated." << endl;
                break;

            case 2: // Due Date
                cout << "Enter new due date (YYYY-MM-DD, leave empty to remove): ";
                getline(cin, input);

                if (input.empty()) {
                    task.dueDate = "";
                    cout << "Due date removed." << endl;
                    break;
                }

                if (!isValidDate(input)) {
                    cout << "Invalid date format." << endl;
                    break;
                }

                {
                    string currentDate = getCurrentDate();
                    if (input < currentDate) {
                        cout << "Due date must be today or in the future." << endl;
                        break;
                    }
                }

                task.dueDate = input;
                cout << "Due date updated." << endl;
                break;

            case 3: // Priority
                cout << "Select new priority:" << endl;
                cout << "1. Low" << endl;
                cout << "2. Medium" << endl;
                cout << "3. High" << endl;
                cout << "Enter choice (1-3): ";
                if (!(cin >> choice)) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid input." << endl;
                    break;
                }
                cin.ignore();

                if (choice < 1 || choice > 3) {
                    cout << "Invalid choice." << endl;
                    break;
                }

                task.priority = static_cast<Priority>(choice);
                cout << "Priority updated to " << task.getPriorityString() << endl;
                break;

            case 4: // Status
                cout << "Select new status:" << endl;
                cout << "1. Pending" << endl;
                cout << "2. In Progress" << endl;
                cout << "3. Completed" << endl;
                cout << "Enter choice (1-3): ";
                if (!(cin >> choice)) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid input." << endl;
                    break;
                }
                cin.ignore();

                if (choice < 1 || choice > 3) {
                    cout << "Invalid choice." << endl;
                    break;
                }

                task.status = static_cast<TaskStatus>(choice - 1);
                if (task.status == COMPLETED && task.completedDate == 0) {
                    task.completedDate = time(0);
                }
                cout << "Status updated to " << task.getStatusString() << endl;
                break;

            default:
                cout << "Invalid choice." << endl;
                break;
        }

        save_tasks();
    }

    // Remove task
    void remove_task() {
        if (tasks.empty()) {
            cout << "No tasks available." << endl;
            return;
        }

        int taskIndex;
        view_tasks();
        cout << "Enter task ID to remove: ";
        if (!(cin >> taskIndex)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input." << endl;
            return;
        }
        cin.ignore();

        if (taskIndex < 1 || taskIndex > static_cast<int>(tasks.size())) {
            cout << "Invalid task ID." << endl;
            return;
        }

        cout << "Are you sure you want to remove task \"" << tasks[taskIndex - 1].description << "\"? (y/n): ";
        char confirm;
        cin >> confirm;
        cin.ignore();

        if (tolower(confirm) != 'y') {
            cout << "Task removal cancelled." << endl;
            return;
        }

        tasks.erase(tasks.begin() + taskIndex - 1);
        cout << "Task removed successfully." << endl;
        save_tasks();
    }

    // Sort tasks
    void sort_tasks() {
        if (tasks.empty()) {
            cout << "No tasks available." << endl;
            return;
        }

        int choice;
        cout << "\n=== Sort Tasks ===" << endl;
        cout << "1. By Due Date" << endl;
        cout << "2. By Priority" << endl;
        cout << "3. By Status" << endl;
        cout << "4. By Creation Date" << endl;
        cout << "Enter choice (1-4): ";
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input." << endl;
            return;
        }
        cin.ignore();

        switch (choice) {
            case 1: // By Due Date
                sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
                    if (a.dueDate.empty() && b.dueDate.empty()) return false;
                    if (a.dueDate.empty()) return false;
                    if (b.dueDate.empty()) return true;
                    return a.dueDate < b.dueDate;
                });
                cout << "Tasks sorted by due date." << endl;
                break;

            case 2: // By Priority
                sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
                    return a.priority > b.priority; // High to Low
                });
                cout << "Tasks sorted by priority (High to Low)." << endl;
                break;

            case 3: // By Status
                sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
                    return a.status < b.status; // Pending to Completed
                });
                cout << "Tasks sorted by status (Pending to Completed)." << endl;
                break;

            case 4: // By Creation Date
                sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
                    return a.createdDate > b.createdDate; // Newest to Oldest
                });
                cout << "Tasks sorted by creation date (Newest to Oldest)." << endl;
                break;

            default:
                cout << "Invalid choice." << endl;
                return;
        }

        view_tasks();
    }

    // Filter tasks
    void filter_tasks() const {
        if (tasks.empty()) {
            cout << "No tasks available." << endl;
            return;
        }

        int choice;
        cout << "\n=== Filter Tasks ===" << endl;
        cout << "1. By Status" << endl;
        cout << "2. By Priority" << endl;
        cout << "3. By Due Date (Today)" << endl;
        cout << "4. By Due Date (This Week)" << endl;
        cout << "5. By Due Date (Overdue)" << endl;
        cout << "Enter choice (1-5): ";
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input." << endl;
            return;
        }
        cin.ignore();

        vector<Task> filteredTasks;

        switch (choice) {
            case 1: { // By Status
                int statusChoice;
                cout << "Select status:" << endl;
                cout << "1. Pending" << endl;
                cout << "2. In Progress" << endl;
                cout << "3. Completed" << endl;
                cout << "Enter choice (1-3): ";
                if (!(cin >> statusChoice)) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid input." << endl;
                    return;
                }
                cin.ignore();

                if (statusChoice < 1 || statusChoice > 3) {
                    cout << "Invalid choice." << endl;
                    return;
                }

                TaskStatus status = static_cast<TaskStatus>(statusChoice - 1);
                for (const auto& task : tasks) {
                    if (task.status == status) {
                        filteredTasks.push_back(task);
                    }
                }

                cout << "Tasks with status ";
                cout << (status == PENDING ? "Pending" : (status == IN_PROGRESS ? "In Progress" : "Completed")) << ":" << endl;
                break;
            }

            case 2: { // By Priority
                int priorityChoice;
                cout << "Select priority:" << endl;
                cout << "1. Low" << endl;
                cout << "2. Medium" << endl;
                cout << "3. High" << endl;
                cout << "Enter choice (1-3): ";
                if (!(cin >> priorityChoice)) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid input." << endl;
                    return;
                }
                cin.ignore();

                if (priorityChoice < 1 || priorityChoice > 3) {
                    cout << "Invalid choice." << endl;
                    return;
                }

                Priority priority = static_cast<Priority>(priorityChoice);
                for (const auto& task : tasks) {
                    if (task.priority == priority) {
                        filteredTasks.push_back(task);
                    }
                }

                cout << "Tasks with " << (priority == LOW ? "Low" : (priority == MEDIUM ? "Medium" : "High")) << " priority:" << endl;
                break;
            }

            case 3: { // Due Today
                string currentDate = getCurrentDate();
                for (const auto& task : tasks) {
                    if (task.dueDate == currentDate) {
                        filteredTasks.push_back(task);
                    }
                }

                cout << "Tasks due today (" << currentDate << "):" << endl;
                break;
            }

            case 4: { // Due This Week
                string currentDate = getCurrentDate();

                time_t now = time(0);
                struct tm timeinfo = *localtime(&now);
                timeinfo.tm_mday += 7;
                mktime(&timeinfo); // Normalize the date

                char weekLater[11];
                strftime(weekLater, sizeof(weekLater), "%Y-%m-%d", &timeinfo);

                for (const auto& task : tasks) {
                    if (!task.dueDate.empty() && task.dueDate >= currentDate && task.dueDate <= weekLater) {
                        filteredTasks.push_back(task);
                    }
                }

                cout << "Tasks due this week (" << currentDate << " to " << weekLater << "):" << endl;
                break;
            }

            case 5: { // Overdue
                string currentDate = getCurrentDate();
                for (const auto& task : tasks) {
                    if (!task.dueDate.empty() && task.dueDate < currentDate && task.status != COMPLETED) {
                        filteredTasks.push_back(task);
                    }
                }

                cout << "Overdue tasks:" << endl;
                break;
            }

            default:
                cout << "Invalid choice." << endl;
                return;
        }

        if (filteredTasks.empty()) {
            cout << "No tasks match the filter criteria." << endl;
            return;
        }

        cout << left << setw(5) << "ID"
             << setw(30) << "Description"
             << setw(12) << "Due Date"
             << setw(10) << "Priority"
             << setw(15) << "Status" << endl;
        cout << string(72, '-') << endl;

        for (size_t i = 0; i < filteredTasks.size(); ++i) {
            cout << left << setw(5) << (i + 1)
                 << setw(30) << (filteredTasks[i].description.length() > 27 ?
                                 filteredTasks[i].description.substr(0, 27) + "..." :
                                 filteredTasks[i].description)
                 << setw(12) << (filteredTasks[i].dueDate.empty() ? "None" : filteredTasks[i].dueDate)
                 << setw(10) << filteredTasks[i].getPriorityString()
                 << setw(15) << filteredTasks[i].getStatusString() << endl;
        }

        cout << endl;
    }

    // Get statistics
    void show_statistics() const {
        if (tasks.empty()) {
            cout << "No tasks available." << endl;
            return;
        }

        int pending = 0, inProgress = 0, completed = 0;
        int low = 0, medium = 0, high = 0;
        int overdue = 0;
        string currentDate = getCurrentDate();

        for (const auto& task : tasks) {
            switch (task.status) {
                case PENDING: pending++; break;
                case IN_PROGRESS: inProgress++; break;
                case COMPLETED: completed++; break;
            }

            switch (task.priority) {
                case LOW: low++; break;
                case MEDIUM: medium++; break;
                case HIGH: high++; break;
            }

            if (!task.dueDate.empty() && task.dueDate < currentDate && task.status != COMPLETED) {
                overdue++;
            }
        }

        cout << "\n=== Task Statistics ===" << endl;
        cout << "Total Tasks: " << tasks.size() << endl;
        cout << "By Status:" << endl;
        cout << "  Pending: " << pending << " (" << (pending * 100 / tasks.size()) << "%)" << endl;
        cout << "  In Progress: " << inProgress << " (" << (inProgress * 100 / tasks.size()) << "%)" << endl;
        cout << "  Completed: " << completed << " (" << (completed * 100 / tasks.size()) << "%)" << endl;
        cout << "By Priority:" << endl;
        cout << "  Low: " << low << " (" << (low * 100 / tasks.size()) << "%)" << endl;
        cout << "  Medium: " << medium << " (" << (medium * 100 / tasks.size()) << "%)" << endl;
        cout << "  High: " << high << " (" << (high * 100 / tasks.size()) << "%)" << endl;
        cout << "Overdue Tasks: " << overdue << endl;
        cout << endl;
    }

    // Logout
    void logout() {
        save_tasks();
        isLoggedIn = false;
        userID = "";
        tasks.clear();
        cout << "Logged out successfully." << endl;
    }

    // Check if user is logged in
    bool is_logged_in() const {
        return isLoggedIn;
    }
};

// Display main menu
void display_main_menu() {
    cout << "\n=== To-Do List Manager ===" << endl;
    cout << "1. Create Account" << endl;
    cout << "2. Log In" << endl;
    cout << "3. Exit" << endl;
    cout << "Enter your choice: ";
}

// Display task menu
void display_task_menu() {
    cout << "\n=== Task Menu ===" << endl;
    cout << "1. Add Task" << endl;
    cout << "2. View Tasks" << endl;
    cout << "3. View Task Details" << endl;
    cout << "4. Update Task Status" << endl;
    cout << "5. Edit Task" << endl;
    cout << "6. Remove Task" << endl;
    cout << "7. Sort Tasks" << endl;
    cout << "8. Filter Tasks" << endl;
    cout << "9. Show Statistics" << endl;
    cout << "10. Logout" << endl;
    cout << "Enter your choice: ";
}

int main() {
    TaskManager manager;
    int choice;

    cout << "\t\t\tWelcome to the To-Do List Manager" << endl;
    cout << "\t\t******************************************" << endl;

    while (true) {
        if (!manager.is_logged_in()) {
            display_main_menu();
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a number." << endl;
                continue;
            }
            cin.ignore();

            switch (choice) {
                case 1:
                    manager.create_account();
                    break;
                case 2:
                    manager.login();
                    break;
                case 3:
                    cout << "Thank you for using To-Do List Manager. Goodbye!" << endl;
                    return 0;
                default:
                    cout << "Invalid choice. Please try again." << endl;
                    break;
            }
        } else {
            display_task_menu();
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a number." << endl;
                continue;
            }
            cin.ignore();

            switch (choice) {
                case 1:
                    manager.add_task();
                    break;
                case 2:
                    manager.view_tasks();
                    break;
                case 3:
                    manager.view_task_details();
                    break;
                case 4:
                    manager.update_task_status();
                    break;
                case 5:
                    manager.edit_task();
                    break;
                case 6:
                    manager.remove_task();
                    break;
                case 7:
                    manager.sort_tasks();
                    break;
                case 8:
                    manager.filter_tasks();
                    break;
                case 9:
                    manager.show_statistics();
                    break;
                case 10:
                    manager.logout();
                    break;
                default:
                    cout << "Invalid choice. Please try again." << endl;
                    break;
            }
        }
    }

    return 0;
}
