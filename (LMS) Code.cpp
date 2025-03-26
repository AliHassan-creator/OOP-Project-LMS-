#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include <map>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <stdexcept>
#include <limits>

using namespace std;

// Constants
const int MAX_BORROW_LIMIT = 5;
const int MAX_REVIEW_LENGTH = 500;
const int MAX_BOOKS_IN_LIBRARY = 10000;
const int MAX_USERS = 1000;
const int MAX_ADMINS = 50;
const int MAX_LOGIN_ATTEMPTS = 3;
const int SESSION_TIMEOUT_MINUTES = 30;
const double LATE_FEE_PER_DAY = 0.50;
const int MAX_BORROW_DAYS = 14;
const int PREMIUM_BORROW_DAYS = 21;

// Forward declarations
class Book;
class User;
class Admin;
class Library;
class Transaction;
class NotificationSystem;

// Utility functions
namespace LibraryUtils {
    string getCurrentDateTime() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
        return string(buffer);
    }

    string getCurrentDate() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", ltm);
        return string(buffer);
    }

    bool validatePassword(const string& password) {
        if (password.length() < 8) return false;
        bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
        for (char c : password) {
            if (isupper(c)) hasUpper = true;
            if (islower(c)) hasLower = true;
            if (isdigit(c)) hasDigit = true;
            if (ispunct(c)) hasSpecial = true;
        }
        return hasUpper && hasLower && hasDigit && hasSpecial;
    }

    bool validateEmail(const string& email) {
        size_t at = email.find('@');
        if (at == string::npos) return false;
        size_t dot = email.find('.', at);
        return dot != string::npos && dot > at + 1 && dot < email.length() - 1;
    }

    string generateISBN() {
        static random_device rd;
        static mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 9);
        string isbn;
        for (int i = 0; i < 13; ++i) {
            isbn += to_string(dis(gen));
        }
        return isbn;
    }

    int daysBetweenDates(const string& date1, const string& date2) {
        tm tm1 = {}, tm2 = {};
        istringstream iss1(date1), iss2(date2);
        iss1 >> get_time(&tm1, "%Y-%m-%d");
        iss2 >> get_time(&tm2, "%Y-%m-%d");
        time_t time1 = mktime(&tm1);
        time_t time2 = mktime(&tm2);
        return difftime(time2, time1) / (60 * 60 * 24);
    }

    string toLower(const string& str) {
        string lowerStr;
        for (char c : str) {
            lowerStr += tolower(c);
        }
        return lowerStr;
    }

    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (string::npos == first) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, (last - first + 1));
    }
}

// Enums
enum class BookFormat {
    HARDCOVER,
    PAPERBACK,
    EBOOK_PDF,
    EBOOK_EPUB,
    EBOOK_MOBI,
    AUDIOBOOK
};

enum class BookStatus {
    AVAILABLE,
    BORROWED,
    RESERVED,
    LOST,
    DAMAGED,
    UNDER_MAINTENANCE
};

enum class UserType {
    STANDARD,
    PREMIUM,
    STUDENT,
    FACULTY,
    STAFF,
    GUEST
};

enum class NotificationType {
    DUE_DATE_REMINDER,
    OVERDUE_NOTICE,
    RESERVATION_AVAILABLE,
    NEW_BOOK_ARRIVAL,
    GENERAL_ANNOUNCEMENT
};

// Book class hierarchy
class Book {
protected:
    string title;
    string author;
    int id;
    string publicationDate;
    string isbn;
    vector<string> reviews;
    vector<string> reviewAuthors;
    vector<string> reviewDates;
    int borrowCount;
    BookStatus status;
    vector<string> borrowHistory;
    set<string> reservedBy;
    string publisher;
    string language;
    string description;
    vector<string> tags;
    double rating;
    int ratingCount;
    string location;
    string edition;
    int year;
    vector<string> similarBooks;

public:
    Book(string t, string a, int i, string isbn, string pubDate, 
         string pub = "Unknown", string lang = "English", string desc = "",
         string loc = "General", string ed = "1st", int y = 0)
        : title(t), author(a), id(i), isbn(isbn), publicationDate(pubDate),
          borrowCount(0), status(BookStatus::AVAILABLE), publisher(pub),
          language(lang), description(desc), location(loc), edition(ed), year(y),
          rating(0), ratingCount(0) {
        if (isbn.length() != 10 && isbn.length() != 13) {
            throw invalid_argument("ISBN must be 10 or 13 digits");
        }
        if (year == 0) {
            // Extract year from publication date if not provided
            try {
                year = stoi(pubDate.substr(0, 4));
            } catch (...) {
                year = 1900;
            }
        }
    }

    virtual ~Book() {}

    // Pure virtual functions
    virtual void displayInfo() const = 0;
    virtual string getBookType() const = 0;
    virtual string getGenre() const = 0;
    virtual int calculateReadingTime() const = 0;
    virtual BookFormat getFormat() const = 0;

    // Common book operations
    string getTitle() const { return title; }
    string getAuthor() const { return author; }
    int getId() const { return id; }
    string getISBN() const { return isbn; }
    string getPublicationDate() const { return publicationDate; }
    BookStatus getStatus() const { return status; }
    int getBorrowCount() const { return borrowCount; }
    string getPublisher() const { return publisher; }
    string getLanguage() const { return language; }
    string getDescription() const { return description; }
    string getLocation() const { return location; }
    string getEdition() const { return edition; }
    int getYear() const { return year; }
    double getRating() const { return ratingCount > 0 ? rating / ratingCount : 0; }

    void addReview(const string& review, const string& username, int rating) {
        if (review.length() > MAX_REVIEW_LENGTH) {
            cout << "Review exceeds maximum length of " << MAX_REVIEW_LENGTH << " characters.\n";
            return;
        }
        if (rating < 1 || rating > 5) {
            cout << "Rating must be between 1 and 5.\n";
            return;
        }
        reviews.push_back(review);
        reviewAuthors.push_back(username);
        reviewDates.push_back(LibraryUtils::getCurrentDateTime());
        this->rating += rating;
        ratingCount++;
        cout << "Review added by " << username << " on " << reviewDates.back() << ":\n";
        cout << "\"" << review << "\" (Rating: " << rating << "/5)\n";
    }

    void displayReviews() const {
        if (reviews.empty()) {
            cout << "No reviews available for " << title << ".\n";
            return;
        }
        cout << "Reviews for \"" << title << "\" (Average Rating: " 
             << fixed << setprecision(1) << getRating() << "/5):\n";
        cout << "----------------------------------------\n";
        for (size_t i = 0; i < reviews.size(); ++i) {
            cout << "Review #" << i + 1 << " by " << reviewAuthors[i] 
                 << " (" << reviewDates[i] << "):\n";
            cout << "Rating: " << (static_cast<int>(rating) / ratingCount) << "/5\n";
            cout << reviews[i] << "\n\n";
        }
        cout << "----------------------------------------\n";
    }

    void updateStatus(BookStatus newStatus) {
        status = newStatus;
        string statusStr;
        switch (status) {
            case BookStatus::AVAILABLE: statusStr = "Available"; break;
            case BookStatus::BORROWED: statusStr = "Borrowed"; break;
            case BookStatus::RESERVED: statusStr = "Reserved"; break;
            case BookStatus::LOST: statusStr = "Lost"; break;
            case BookStatus::DAMAGED: statusStr = "Damaged"; break;
            case BookStatus::UNDER_MAINTENANCE: statusStr = "Under Maintenance"; break;
        }
        cout << title << " status changed to: " << statusStr << ".\n";
    }

    void recordBorrow(const string& username) {
        borrowCount++;
        borrowHistory.push_back(username + " borrowed on " + LibraryUtils::getCurrentDateTime());
        updateStatus(BookStatus::BORROWED);
    }

    void recordReturn(const string& username) {
        borrowHistory.push_back(username + " returned on " + LibraryUtils::getCurrentDateTime());
        updateStatus(BookStatus::AVAILABLE);
    }

    bool reserve(const string& username) {
        if (status != BookStatus::AVAILABLE) {
            cout << "Book is not available for reservation.\n";
            return false;
        }
        if (reservedBy.count(username) {
            cout << "You have already reserved this book.\n";
            return false;
        }
        reservedBy.insert(username);
        if (reservedBy.size() == 1) {
            updateStatus(BookStatus::RESERVED);
        }
        cout << "Book reserved successfully for " << username << ".\n";
        return true;
    }

    bool cancelReservation(const string& username) {
        if (!reservedBy.count(username)) {
            cout << "No reservation found for this user.\n";
            return false;
        }
        reservedBy.erase(username);
        if (reservedBy.empty()) {
            updateStatus(BookStatus::AVAILABLE);
        }
        cout << "Reservation canceled successfully for " << username << ".\n";
        return true;
    }

    void displayBorrowHistory() const {
        cout << "Borrow history for \"" << title << "\":\n";
        cout << "----------------------------------------\n";
        for (const auto& record : borrowHistory) {
            cout << "- " << record << "\n";
        }
        cout << "----------------------------------------\n";
    }

    virtual void printDetailedInfo() const {
        cout << "Detailed Information for \"" << title << "\":\n";
        cout << "----------------------------------------\n";
        cout << "Author: " << author << "\n";
        cout << "ID: " << id << "\n";
        cout << "ISBN: " << isbn << "\n";
        cout << "Publication Date: " << publicationDate << "\n";
        cout << "Publisher: " << publisher << "\n";
        cout << "Language: " << language << "\n";
        cout << "Edition: " << edition << "\n";
        cout << "Year: " << year << "\n";
        cout << "Type: " << getBookType() << "\n";
        cout << "Format: ";
        switch (getFormat()) {
            case BookFormat::HARDCOVER: cout << "Hardcover"; break;
            case BookFormat::PAPERBACK: cout << "Paperback"; break;
            case BookFormat::EBOOK_PDF: cout << "E-book (PDF)"; break;
            case BookFormat::EBOOK_EPUB: cout << "E-book (EPUB)"; break;
            case BookFormat::EBOOK_MOBI: cout << "E-book (MOBI)"; break;
            case BookFormat::AUDIOBOOK: cout << "Audiobook"; break;
        }
        cout << "\n";
        cout << "Genre: " << getGenre() << "\n";
        cout << "Status: ";
        switch (status) {
            case BookStatus::AVAILABLE: cout << "Available"; break;
            case BookStatus::BORROWED: cout << "Borrowed"; break;
            case BookStatus::RESERVED: cout << "Reserved"; break;
            case BookStatus::LOST: cout << "Lost"; break;
            case BookStatus::DAMAGED: cout << "Damaged"; break;
            case BookStatus::UNDER_MAINTENANCE: cout << "Under Maintenance"; break;
        }
        cout << "\n";
        cout << "Times borrowed: " << borrowCount << "\n";
        cout << "Location: " << location << "\n";
        cout << "Average Rating: " << fixed << setprecision(1) << getRating() << "/5 (" 
             << ratingCount << " ratings)\n";
        cout << "Estimated reading time: " << calculateReadingTime() << " minutes\n";
        if (!description.empty()) {
            cout << "\nDescription:\n" << description << "\n";
        }
        if (!tags.empty()) {
            cout << "\nTags: ";
            for (const auto& tag : tags) cout << tag << ", ";
            cout << "\n";
        }
        if (!reservedBy.empty()) {
            cout << "\nReserved by (" << reservedBy.size() << "): ";
            for (const auto& user : reservedBy) cout << user << ", ";
            cout << "\n";
        }
        cout << "----------------------------------------\n";
    }

    void addTag(const string& tag) {
        string trimmedTag = LibraryUtils::trim(tag);
        if (!trimmedTag.empty()) {
            tags.push_back(trimmedTag);
        }
    }

    bool hasTag(const string& tag) const {
        string lowerTag = LibraryUtils::toLower(tag);
        for (const auto& t : tags) {
            if (LibraryUtils::toLower(t) == lowerTag) {
                return true;
            }
        }
        return false;
    }

    void setDescription(const string& desc) {
        description = desc;
    }

    void setLocation(const string& loc) {
        location = loc;
    }
};

// Derived Book Classes
class FictionBook : public Book {
protected:
    string subgenre;
    bool isSeries;
    string seriesName;
    int seriesNumber;

public:
    FictionBook(string t, string a, int i, string isbn, string pubDate, 
                string subg, string pub = "Unknown", string lang = "English", 
                string desc = "", string loc = "Fiction", string ed = "1st", int y = 0,
                bool series = false, string sName = "", int sNum = 0)
        : Book(t, a, i, isbn, pubDate, pub, lang, desc, loc, ed, y),
          subgenre(subg), isSeries(series), seriesName(sName), seriesNumber(sNum) {}

    string getSubgenre() const { return subgenre; }
    bool getIsSeries() const { return isSeries; }
    string getSeriesName() const { return seriesName; }
    int getSeriesNumber() const { return seriesNumber; }

    void printDetailedInfo() const override {
        Book::printDetailedInfo();
        cout << "Subgenre: " << subgenre << "\n";
        if (isSeries) {
            cout << "Part of series: " << seriesName << " (Book #" << seriesNumber << ")\n";
        }
    }
};

class NonFictionBook : public Book {
protected:
    string subject;
    string classification;

public:
    NonFictionBook(string t, string a, int i, string isbn, string pubDate, 
                   string subj, string cls, string pub = "Unknown", 
                   string lang = "English", string desc = "", 
                   string loc = "Non-Fiction", string ed = "1st", int y = 0)
        : Book(t, a, i, isbn, pubDate, pub, lang, desc, loc, ed, y),
          subject(subj), classification(cls) {}

    string getSubject() const { return subject; }
    string getClassification() const { return classification; }

    void printDetailedInfo() const override {
        Book::printDetailedInfo();
        cout << "Subject: " << subject << "\n";
        cout << "Classification: " << classification << "\n";
    }
};

class EBook : public Book {
private:
    BookFormat format;
    double fileSizeMB;
    int wordCount;
    bool drmProtected;
    string downloadLink;
    vector<string> compatibleDevices;

public:
    EBook(string t, string a, int i, string isbn, string pubDate, 
          BookFormat f, double size, int words, bool drm = false,
          string link = "", string pub = "Unknown", string lang = "English",
          string desc = "", string loc = "Digital", string ed = "1st", int y = 0)
        : Book(t, a, i, isbn, pubDate, pub, lang, desc, loc, ed, y),
          format(f), fileSizeMB(size), wordCount(words), drmProtected(drm),
          downloadLink(link) {
        compatibleDevices = {"Computer", "Tablet", "Smartphone", "E-reader"};
    }

    void displayInfo() const override {
        cout << "[E-Book] " << title << " by " << author << "\n";
        cout << "  Format: ";
        switch (format) {
            case BookFormat::EBOOK_PDF: cout << "PDF"; break;
            case BookFormat::EBOOK_EPUB: cout << "EPUB"; break;
            case BookFormat::EBOOK_MOBI: cout << "MOBI"; break;
            default: cout << "Unknown";
        }
        cout << " | Size: " << fileSizeMB << "MB | Words: " << wordCount << "\n";
        cout << "  ISBN: " << isbn << " | Published: " << publicationDate << "\n";
    }

    string getBookType() const override { return "E-Book"; }
    string getGenre() const override { return "Digital"; }
    BookFormat getFormat() const override { return format; }
    
    int calculateReadingTime() const override {
        // Average reading speed: 200 words per minute
        return wordCount / 200 + 1;
    }

    void printDetailedInfo() const override {
        Book::printDetailedInfo();
        cout << "Format: ";
        switch (format) {
            case BookFormat::EBOOK_PDF: cout << "PDF"; break;
            case BookFormat::EBOOK_EPUB: cout << "EPUB"; break;
            case BookFormat::EBOOK_MOBI: cout << "MOBI"; break;
            default: cout << "Unknown";
        }
        cout << "\n";
        cout << "File Size: " << fileSizeMB << " MB\n";
        cout << "Word Count: " << wordCount << "\n";
        cout << "DRM Protected: " << (drmProtected ? "Yes" : "No") << "\n";
        if (!downloadLink.empty()) {
            cout << "Download Link: " << downloadLink << "\n";
        }
        cout << "Compatible Devices: ";
        for (const auto& device : compatibleDevices) cout << device << ", ";
        cout << "\n";
    }

    void setDownloadLink(const string& link) {
        downloadLink = link;
    }

    void addCompatibleDevice(const string& device) {
        compatibleDevices.push_back(device);
    }
};

class PrintedBook : public Book {
private:
    BookFormat format;
    int pages;
    string bindingType;
    string dimensions;
    double weight;
    bool hasIllustrations;
    string condition;

public:
    PrintedBook(string t, string a, int i, string isbn, string pubDate, 
                BookFormat f, int p, string binding, string dim, double w,
                bool illus = false, string cond = "Good", string pub = "Unknown",
                string lang = "English", string desc = "", string loc = "Stacks",
                string ed = "1st", int y = 0)
        : Book(t, a, i, isbn, pubDate, pub, lang, desc, loc, ed, y),
          format(f), pages(p), bindingType(binding), dimensions(dim), weight(w),
          hasIllustrations(illus), condition(cond) {}

    void displayInfo() const override {
        cout << "[Printed Book] " << title << " by " << author << "\n";
        cout << "  Format: ";
        switch (format) {
            case BookFormat::HARDCOVER: cout << "Hardcover"; break;
            case BookFormat::PAPERBACK: cout << "Paperback"; break;
            default: cout << "Unknown";
        }
        cout << " | Pages: " << pages << " | Binding: " << bindingType << "\n";
        cout << "  ISBN: " << isbn << " | Published: " << publicationDate << "\n";
    }

    string getBookType() const override { return "Printed Book"; }
    string getGenre() const override { return "Physical"; }
    BookFormat getFormat() const override { return format; }
    
    int calculateReadingTime() const override {
        // Average reading speed: 1 page per 2 minutes
        return pages * 2;
    }

    void printDetailedInfo() const override {
        Book::printDetailedInfo();
        cout << "Format: ";
        switch (format) {
            case BookFormat::HARDCOVER: cout << "Hardcover"; break;
            case BookFormat::PAPERBACK: cout << "Paperback"; break;
            default: cout << "Unknown";
        }
        cout << "\n";
        cout << "Pages: " << pages << "\n";
        cout << "Binding: " << bindingType << "\n";
        cout << "Dimensions: " << dimensions << "\n";
        cout << "Weight: " << weight << " grams\n";
        cout << "Illustrations: " << (hasIllustrations ? "Yes" : "No") << "\n";
        cout << "Condition: " << condition << "\n";
    }

    void updateCondition(const string& newCondition) {
        condition = newCondition;
        if (newCondition == "Poor" || newCondition == "Damaged") {
            updateStatus(BookStatus::DAMAGED);
        }
    }
};

class FantasyNovel : public FictionBook {
private:
    bool hasMagicSystem;
    string worldName;
    vector<string> magicalCreatures;

public:
    FantasyNovel(string t, string a, int i, string isbn, string pubDate, 
                 string subg, bool magic = false, string world = "",
                 string pub = "Unknown", string lang = "English", 
                 string desc = "", string loc = "Fantasy", string ed = "1st", 
                 int y = 0, bool series = false, string sName = "", int sNum = 0)
        : FictionBook(t, a, i, isbn, pubDate, subg, pub, lang, desc, loc, ed, y,
                     series, sName, sNum),
          hasMagicSystem(magic), worldName(world) {}

    void displayInfo() const override {
        cout << "[Fantasy Novel] " << getTitle() << " by " << getAuthor() << "\n";
        cout << "  Subgenre: " << subgenre;
        if (isSeries) {
            cout << " | Series: " << seriesName << " #" << seriesNumber;
        }
        if (!worldName.empty()) {
            cout << " | World: " << worldName;
        }
        cout << "\n";
    }

    string getBookType() const override { return "Fantasy Novel"; }
    string getGenre() const override { return "Fantasy/" + subgenre; }
    BookFormat getFormat() const override { return BookFormat::PAPERBACK; }
    
    int calculateReadingTime() const override {
        // Fantasy novels might take longer to read
        return pages * 3;
    }

    void printDetailedInfo() const override {
        FictionBook::printDetailedInfo();
        cout << "Magic System: " << (hasMagicSystem ? "Yes" : "No") << "\n";
        if (!worldName.empty()) {
            cout << "World Name: " << worldName << "\n";
        }
        if (!magicalCreatures.empty()) {
            cout << "Magical Creatures: ";
            for (const auto& creature : magicalCreatures) cout << creature << ", ";
            cout << "\n";
        }
    }

    void addMagicalCreature(const string& creature) {
        magicalCreatures.push_back(creature);
    }

    void setWorldName(const string& world) {
        worldName = world;
    }
};

class ScienceTextbook : public NonFictionBook {
private:
    string field;
    int editionYear;
    vector<string> authors;
    bool hasExercises;
    string courseCode;

public:
    ScienceTextbook(string t, string a, int i, string isbn, string pubDate, 
                    string subj, string field, string cls, int edYear,
                    string pub = "Academic Press", string lang = "English",
                    string desc = "", string loc = "Textbooks", string ed = "1st",
                    int y = 0, bool exercises = true, string code = "")
        : NonFictionBook(t, a, i, isbn, pubDate, subj, cls, pub, lang, desc, loc, ed, y),
          field(field), editionYear(edYear), hasExercises(exercises), courseCode(code) {
        // Split multiple authors if separated by commas
        size_t pos = 0;
        string token;
        string authorsStr = a;
        while ((pos = authorsStr.find(',')) != string::npos) {
            token = authorsStr.substr(0, pos);
            authors.push_back(LibraryUtils::trim(token));
            authorsStr.erase(0, pos + 1);
        }
        authors.push_back(LibraryUtils::trim(authorsStr));
    }

    void displayInfo() const override {
        cout << "[Science Textbook] " << title << "\n";
        cout << "  Field: " << field << " | Subject: " << subject << "\n";
        cout << "  Authors: ";
        for (size_t i = 0; i < authors.size(); ++i) {
            if (i > 0) cout << ", ";
            cout << authors[i];
        }
        cout << "\n";
    }

    string getBookType() const override { return "Science Textbook"; }
    string getGenre() const override { return "Education/" + field; }
    BookFormat getFormat() const override { return BookFormat::HARDCOVER; }
    
    int calculateReadingTime() const override {
        // Textbooks take longer per page
        return pages * 5;
    }

    void printDetailedInfo() const override {
        NonFictionBook::printDetailedInfo();
        cout << "Field: " << field << "\n";
        cout << "Edition Year: " << editionYear << "\n";
        cout << "Authors: ";
        for (size_t i = 0; i < authors.size(); ++i) {
            if (i > 0) cout << ", ";
            cout << authors[i];
        }
        cout << "\n";
        cout << "Exercises: " << (hasExercises ? "Yes" : "No") << "\n";
        if (!courseCode.empty()) {
            cout << "Course Code: " << courseCode << "\n";
        }
    }

    void addAuthor(const string& author) {
        authors.push_back(author);
    }

    void setCourseCode(const string& code) {
        courseCode = code;
    }
};

// Transaction class
class Transaction {
private:
    int transactionId;
    string username;
    int bookId;
    string transactionDate;
    string dueDate;
    string returnDate;
    double lateFee;
    bool isReturned;
    string transactionType; // "borrow", "return", "renew", "reserve"

public:
    Transaction(int id, string user, int book, string type, 
                string date = "", string due = "")
        : transactionId(id), username(user), bookId(book), transactionType(type),
          lateFee(0.0), isReturned(false) {
        transactionDate = date.empty() ? LibraryUtils::getCurrentDateTime() : date;
        
        if (type == "borrow") {
            int days = MAX_BORROW_DAYS;
            dueDate = due.empty() ? LibraryUtils::getCurrentDate() : due;
            // Add days to due date (simplified)
            dueDate = LibraryUtils::getCurrentDate(); // This should be properly calculated
        }
    }

    void calculateLateFee() {
        if (isReturned || transactionType != "borrow") return;
        
        string currentDate = LibraryUtils::getCurrentDate();
        int daysLate = LibraryUtils::daysBetweenDates(dueDate, currentDate);
        
        if (daysLate > 0) {
            lateFee = daysLate * LATE_FEE_PER_DAY;
        }
    }

    void markReturned(string date = "") {
        returnDate = date.empty() ? LibraryUtils::getCurrentDateTime() : date;
        isReturned = true;
        calculateLateFee();
    }

    void displayInfo() const {
        cout << "Transaction #" << transactionId << " (" << transactionType << ")\n";
        cout << "----------------------------------------\n";
        cout << "User: " << username << "\n";
        cout << "Book ID: " << bookId << "\n";
        cout << "Date: " << transactionDate << "\n";
        if (transactionType == "borrow") {
            cout << "Due Date: " << dueDate << "\n";
            cout << "Returned: " << (isReturned ? "Yes" : "No") << "\n";
            if (isReturned) {
                cout << "Return Date: " << returnDate << "\n";
                if (lateFee > 0) {
                    cout << "Late Fee: $" << fixed << setprecision(2) << lateFee << "\n";
                }
            }
        }
        cout << "----------------------------------------\n";
    }

    int getId() const { return transactionId; }
    string getUsername() const { return username; }
    int getBookId() const { return bookId; }
    string getType() const { return transactionType; }
    string getDueDate() const { return dueDate; }
    double getLateFee() const { return lateFee; }
    bool getIsReturned() const { return isReturned; }

    void renew(int additionalDays) {
        if (transactionType != "borrow" || isReturned) return;
        
        // Simplified date calculation
        dueDate = LibraryUtils::getCurrentDate(); // Should properly add days
        cout << "Transaction #" << transactionId << " renewed. New due date: " 
             << dueDate << "\n";
    }
};

// Notification System
class NotificationSystem {
private:
    struct Notification {
        int id;
        string recipient;
        string message;
        string date;
        NotificationType type;
        bool isRead;
    };

    vector<Notification> notifications;
    int nextId;

public:
    NotificationSystem() : nextId(1) {}

    void sendNotification(const string& recipient, const string& message, 
                         NotificationType type) {
        notifications.push_back({
            nextId++,
            recipient,
            message,
            LibraryUtils::getCurrentDateTime(),
            type,
            false
        });
    }

    void markAsRead(int notificationId) {
        for (auto& note : notifications) {
            if (note.id == notificationId) {
                note.isRead = true;
                break;
            }
        }
    }

    vector<Notification> getUnreadNotifications(const string& username) const {
        vector<Notification> unread;
        for (const auto& note : notifications) {
            if (note.recipient == username && !note.isRead) {
                unread.push_back(note);
            }
        }
        return unread;
    }

    vector<Notification> getAllNotifications(const string& username) const {
        vector<Notification> userNotes;
        for (const auto& note : notifications) {
            if (note.recipient == username) {
                userNotes.push_back(note);
            }
        }
        return userNotes;
    }

    void displayNotifications(const string& username) const {
        auto userNotes = getAllNotifications(username);
        if (userNotes.empty()) {
            cout << "No notifications found.\n";
            return;
        }

        cout << "Notifications for " << username << ":\n";
        cout << "----------------------------------------\n";
        for (const auto& note : userNotes) {
            cout << "[" << note.date << "] ";
            switch (note.type) {
                case NotificationType::DUE_DATE_REMINDER: cout << "REMINDER: "; break;
                case NotificationType::OVERDUE_NOTICE: cout << "OVERDUE: "; break;
                case NotificationType::RESERVATION_AVAILABLE: cout << "RESERVATION: "; break;
                case NotificationType::NEW_BOOK_ARRIVAL: cout << "NEW BOOK: "; break;
                case NotificationType::GENERAL_ANNOUNCEMENT: cout << "ANNOUNCEMENT: "; break;
            }
            cout << note.message << "\n";
            cout << (note.isRead ? "(read)" : "(new)") << "\n\n";
        }
        cout << "----------------------------------------\n";
    }

    void checkDueDates(const vector<Transaction>& transactions) {
        string today = LibraryUtils::getCurrentDate();
        for (const auto& trans : transactions) {
            if (trans.getType() == "borrow" && !trans.getIsReturned()) {
                int daysRemaining = LibraryUtils::daysBetweenDates(today, trans.getDueDate());
                
                if (daysRemaining == 1) {
                    sendNotification(trans.getUsername(), 
                        "Your borrowed book (ID: " + to_string(trans.getBookId()) + 
                        ") is due tomorrow.", NotificationType::DUE_DATE_REMINDER);
                } else if (daysRemaining < 0) {
                    sendNotification(trans.getUsername(), 
                        "Your borrowed book (ID: " + to_string(trans.getBookId()) + 
                        ") is overdue by " + to_string(-daysRemaining) + " days.", 
                        NotificationType::OVERDUE_NOTICE);
                }
            }
        }
    }
};

// User Management
class User {
private:
    string username;
    string password;
    string fullName;
    string email;
    string joinDate;
    vector<int> borrowedBooks;
    vector<string> borrowingDates;
    vector<string> dueDates;
    vector<string> favoriteGenres;
    int totalBooksBorrowed;
    UserType type;
    double balance;
    vector<int> reservedBooks;
    int loginAttempts;
    string lastLogin;
    bool isActive;
    vector<string> readingHistory;
    map<string, int> genrePreferences;
    vector<string> wishlist;

public:
    User(string u, string p, string name, string email, 
         UserType t = UserType::STANDARD)
        : username(u), password(p), fullName(name), email(email),
          joinDate(LibraryUtils::getCurrentDateTime()), totalBooksBorrowed(0),
          type(t), balance(0.0), loginAttempts(0), isActive(true) {}

    bool authenticate(string u, string p) {
        if (!isActive) {
            cout << "Account is inactive.\n";
            return false;
        }
        if (username == u && password == p) {
            loginAttempts = 0;
            lastLogin = LibraryUtils::getCurrentDateTime();
            return true;
        }
        loginAttempts++;
        if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
            isActive = false;
            cout << "Too many failed login attempts. Account locked.\n";
        }
        return false;
    }

    bool canBorrowMore() const {
        int limit = MAX_BORROW_LIMIT;
        switch (type) {
            case UserType::PREMIUM: limit *= 2; break;
                        case UserType::FACULTY: limit = 10; break;
            case UserType::STAFF: limit = 8; break;
            case UserType::GUEST: limit = 2; break;
            default: break;
        }
        return borrowedBooks.size() < limit;
    }

    int getBorrowLimit() const {
        switch (type) {
            case UserType::PREMIUM: return MAX_BORROW_LIMIT * 2;
            case UserType::FACULTY: return 10;
            case UserType::STAFF: return 8;
            case UserType::GUEST: return 2;
            default: return MAX_BORROW_LIMIT;
        }
    }

    bool borrowBook(int bookID, const string& dueDate = "") {
        if (!canBorrowMore()) {
            cout << "Borrow limit reached (" << getBorrowLimit() << " books). Please return some books first.\n";
            return false;
        }
        if (find(borrowedBooks.begin(), borrowedBooks.end(), bookID) != borrowedBooks.end()) {
            cout << "You've already borrowed this book.\n";
            return false;
        }
        borrowedBooks.push_back(bookID);
        borrowingDates.push_back(LibraryUtils::getCurrentDateTime());
        dueDates.push_back(dueDate.empty() ? LibraryUtils::getCurrentDate() : dueDate);
        totalBooksBorrowed++;
        readingHistory.push_back("Borrowed book ID " + to_string(bookID) + " on " + LibraryUtils::getCurrentDateTime());
        return true;
    }

    bool returnBook(int bookID) {
        auto it = find(borrowedBooks.begin(), borrowedBooks.end(), bookID);
        if (it != borrowedBooks.end()) {
            size_t index = distance(borrowedBooks.begin(), it);
            borrowedBooks.erase(it);
            borrowingDates.erase(borrowingDates.begin() + index);
            dueDates.erase(dueDates.begin() + index);
            readingHistory.push_back("Returned book ID " + to_string(bookID) + " on " + LibraryUtils::getCurrentDateTime());
            cout << "Book ID " << bookID << " returned successfully.\n";
            return true;
        }
        cout << "Book not found in your borrowed list.\n";
        return false;
    }

    bool reserveBook(int bookID) {
        if (find(reservedBooks.begin(), reservedBooks.end(), bookID) != reservedBooks.end()) {
            cout << "You've already reserved this book.\n";
            return false;
        }
        reservedBooks.push_back(bookID);
        readingHistory.push_back("Reserved book ID " + to_string(bookID) + " on " + LibraryUtils::getCurrentDateTime());
        cout << "Book ID " << bookID << " reserved successfully.\n";
        return true;
    }

    bool cancelReservation(int bookID) {
        auto it = find(reservedBooks.begin(), reservedBooks.end(), bookID);
        if (it != reservedBooks.end()) {
            reservedBooks.erase(it);
            readingHistory.push_back("Cancelled reservation for book ID " + to_string(bookID) + " on " + LibraryUtils::getCurrentDateTime());
            cout << "Reservation for book ID " << bookID << " cancelled.\n";
            return true;
        }
        cout << "No reservation found for this book.\n";
        return false;
    }

    void displayBorrowedBooks() const {
        if (borrowedBooks.empty()) {
            cout << "No books currently borrowed.\n";
            return;
        }
        cout << "Books currently borrowed by " << username << ":\n";
        cout << "----------------------------------------\n";
        for (size_t i = 0; i < borrowedBooks.size(); ++i) {
            cout << "- Book ID: " << borrowedBooks[i] 
                 << " (borrowed on " << borrowingDates[i] 
                 << ", due on " << dueDates[i] << ")\n";
        }
        cout << "----------------------------------------\n";
    }

    void displayReservedBooks() const {
        if (reservedBooks.empty()) {
            cout << "No books currently reserved.\n";
            return;
        }
        cout << "Books currently reserved by " << username << ":\n";
        cout << "----------------------------------------\n";
        for (const auto& bookID : reservedBooks) {
            cout << "- Book ID: " << bookID << "\n";
        }
        cout << "----------------------------------------\n";
    }

    void addFavoriteGenre(const string& genre) {
        string lowerGenre = LibraryUtils::toLower(genre);
        if (find(favoriteGenres.begin(), favoriteGenres.end(), lowerGenre) == favoriteGenres.end()) {
            favoriteGenres.push_back(lowerGenre);
            genrePreferences[lowerGenre]++;
            cout << "Added " << genre << " to favorite genres.\n";
        }
    }

    void addToWishlist(const string& bookTitle) {
        if (find(wishlist.begin(), wishlist.end(), bookTitle) == wishlist.end()) {
            wishlist.push_back(bookTitle);
            cout << "Added \"" << bookTitle << "\" to your wishlist.\n";
        }
    }

    void displayProfile() const {
        cout << "\nUser Profile for " << username << ":\n";
        cout << "----------------------------------------\n";
        cout << "Full Name: " << fullName << "\n";
        cout << "Email: " << email << "\n";
        cout << "Member Since: " << joinDate << "\n";
        cout << "Last Login: " << (lastLogin.empty() ? "Never" : lastLogin) << "\n";
        cout << "Account Type: ";
        switch (type) {
            case UserType::STANDARD: cout << "Standard"; break;
            case UserType::PREMIUM: cout << "Premium"; break;
            case UserType::STUDENT: cout << "Student"; break;
            case UserType::FACULTY: cout << "Faculty"; break;
            case UserType::STAFF: cout << "Staff"; break;
            case UserType::GUEST: cout << "Guest"; break;
        }
        cout << "\n";
        cout << "Account Status: " << (isActive ? "Active" : "Inactive") << "\n";
        cout << "Total Books Borrowed: " << totalBooksBorrowed << "\n";
        cout << "Currently Borrowed: " << borrowedBooks.size() << "/" << getBorrowLimit() << " books\n";
        cout << "Currently Reserved: " << reservedBooks.size() << " books\n";
        cout << "Balance Due: $" << fixed << setprecision(2) << balance << "\n";
        cout << "Favorite Genres: ";
        for (const auto& genre : favoriteGenres) cout << genre << ", ";
        cout << "\n";
        if (!wishlist.empty()) {
            cout << "Wishlist: ";
            for (const auto& item : wishlist) cout << "\"" << item << "\", ";
            cout << "\n";
        }
        cout << "----------------------------------------\n";
    }

    void displayReadingHistory(int limit = 10) const {
        if (readingHistory.empty()) {
            cout << "No reading history available.\n";
            return;
        }
        cout << "\nReading History for " << username << " (last " << limit << " entries):\n";
        cout << "----------------------------------------\n";
        int start = max(0, static_cast<int>(readingHistory.size()) - limit);
        for (size_t i = start; i < readingHistory.size(); ++i) {
            cout << "- " << readingHistory[i] << "\n";
        }
        cout << "----------------------------------------\n";
    }

    string getUsername() const { return username; }
    string getEmail() const { return email; }
    UserType getType() const { return type; }
    const vector<int>& getBorrowedBooks() const { return borrowedBooks; }
    const vector<int>& getReservedBooks() const { return reservedBooks; }
    double getBalance() const { return balance; }
    bool getIsActive() const { return isActive; }

    void upgradeAccount(UserType newType) {
        type = newType;
        cout << "Account upgraded to ";
        switch (type) {
            case UserType::PREMIUM: cout << "Premium"; break;
            case UserType::STUDENT: cout << "Student"; break;
            case UserType::FACULTY: cout << "Faculty"; break;
            case UserType::STAFF: cout << "Staff"; break;
            default: break;
        }
        cout << ". Your borrow limit is now " << getBorrowLimit() << " books.\n";
    }

    void addToBalance(double amount) {
        balance += amount;
        if (amount > 0) {
            cout << "$" << fixed << setprecision(2) << amount << " added to your account.\n";
        } else {
            cout << "$" << fixed << setprecision(2) << -amount << " deducted from your account.\n";
        }
        cout << "New balance: $" << fixed << setprecision(2) << balance << "\n";
    }

    void activateAccount() {
        isActive = true;
        loginAttempts = 0;
        cout << "Account activated successfully.\n";
    }

    void updatePassword(const string& newPassword) {
        if (!LibraryUtils::validatePassword(newPassword)) {
            cout << "Password must be at least 8 characters with uppercase, lowercase, numbers and special characters.\n";
            return;
        }
        password = newPassword;
        cout << "Password updated successfully.\n";
    }

    void updateEmail(const string& newEmail) {
        if (!LibraryUtils::validateEmail(newEmail)) {
            cout << "Invalid email format.\n";
            return;
        }
        email = newEmail;
        cout << "Email updated successfully.\n";
    }
};

// Admin class
class Admin {
private:
    string username;
    string password;
    string accessLevel; // "full", "limited", "support"
    string fullName;
    string email;
    string lastLogin;
    int loginAttempts;
    bool isActive;
    vector<string> activityLog;

public:
    Admin(string u, string p, string level = "limited", 
          string name = "", string email = "")
        : username(u), password(p), accessLevel(level),
          fullName(name), email(email), loginAttempts(0), isActive(true) {}

    bool authenticate(string u, string p) {
        if (!isActive) {
            cout << "Account is inactive.\n";
            return false;
        }
        if (username == u && password == p) {
            loginAttempts = 0;
            lastLogin = LibraryUtils::getCurrentDateTime();
            activityLog.push_back("Logged in on " + lastLogin);
            return true;
        }
        loginAttempts++;
        if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
            isActive = false;
            cout << "Too many failed login attempts. Account locked.\n";
        }
        return false;
    }

    bool hasFullAccess() const { return accessLevel == "full"; }
    bool hasLimitedAccess() const { return accessLevel == "limited"; }
    bool hasSupportAccess() const { return accessLevel == "support"; }

    void removeBook(vector<unique_ptr<Book>>& books, int bookID) {
        if (!hasFullAccess() && !hasLimitedAccess()) {
            cout << "You don't have permission to remove books.\n";
            return;
        }
        
        auto it = find_if(books.begin(), books.end(), 
            [bookID](const unique_ptr<Book>& b) { return b->getId() == bookID; });
        
        if (it != books.end()) {
            cout << "Removing book: " << (*it)->getTitle() << endl;
            books.erase(it);
            activityLog.push_back("Removed book ID " + to_string(bookID) + " on " + LibraryUtils::getCurrentDateTime());
        } else {
            cout << "Book ID " << bookID << " not found.\n";
        }
    }

    void addBook(vector<unique_ptr<Book>>& books, unique_ptr<Book> book) {
        if (!hasFullAccess() && !hasLimitedAccess()) {
            cout << "You don't have permission to add books.\n";
            return;
        }
        if (books.size() >= MAX_BOOKS_IN_LIBRARY) {
            cout << "Library capacity reached. Cannot add more books.\n";
            return;
        }
        books.push_back(move(book));
        activityLog.push_back("Added book ID " + to_string(books.back()->getId()) + " on " + LibraryUtils::getCurrentDateTime());
        cout << "Book added successfully.\n";
    }

    void updateBookStatus(Book* book, BookStatus newStatus) {
        if (!hasFullAccess() && !hasLimitedAccess()) {
            cout << "You don't have permission to update book status.\n";
            return;
        }
        book->updateStatus(newStatus);
        activityLog.push_back("Updated status for book ID " + to_string(book->getId()) + " to " + to_string(static_cast<int>(newStatus)) + " on " + LibraryUtils::getCurrentDateTime());
    }

    void manageUserAccount(User& user, bool activate) {
        if (!hasFullAccess() && !hasSupportAccess()) {
            cout << "You don't have permission to manage user accounts.\n";
            return;
        }
        if (activate) {
            user.activateAccount();
        } else {
            // In a real system, we would deactivate here
            cout << "Account deactivation would happen here.\n";
        }
        activityLog.push_back((activate ? "Activated" : "Deactivated") + string(" user ") + user.getUsername() + " on " + LibraryUtils::getCurrentDateTime());
    }

    void displaySystemStats(const vector<unique_ptr<Book>>& books, 
                          const unordered_map<string, User>& users) const {
        if (!hasFullAccess() && !hasLimitedAccess()) {
            cout << "You don't have permission to view system stats.\n";
            return;
        }
        
        cout << "\nLibrary System Statistics:\n";
        cout << "----------------------------------------\n";
        cout << "Total Books: " << books.size() << "\n";
        
        map<string, int> typeCounts;
        map<string, int> genreCounts;
        map<string, int> statusCounts;
        int totalBorrows = 0;
        int availableBooks = 0;
        
        for (const auto& book : books) {
            typeCounts[book->getBookType()]++;
            genreCounts[book->getGenre()]++;
            statusCounts[to_string(static_cast<int>(book->getStatus()))]++;
            totalBorrows += book->getBorrowCount();
            if (book->getStatus() == BookStatus::AVAILABLE) availableBooks++;
        }
        
        cout << "Available Books: " << availableBooks << "\n";
        cout << "\nBooks by Type:\n";
        for (const auto& pair : typeCounts) {
            cout << "- " << pair.first << ": " << pair.second << "\n";
        }
        
        cout << "\nBooks by Genre:\n";
        for (const auto& pair : genreCounts) {
            cout << "- " << pair.first << ": " << pair.second << "\n";
        }
        
        cout << "\nBooks by Status:\n";
        for (const auto& pair : statusCounts) {
            cout << "- " << pair.first << ": " << pair.second << "\n";
        }
        
        cout << "\nTotal Users: " << users.size() << "\n";
        
        map<UserType, int> userTypeCounts;
        int activeUsers = 0;
        for (const auto& pair : users) {
            userTypeCounts[pair.second.getType()]++;
            if (pair.second.getIsActive()) activeUsers++;
        }
        
        cout << "Active Users: " << activeUsers << "\n";
        cout << "\nUsers by Type:\n";
        for (const auto& pair : userTypeCounts) {
            cout << "- ";
            switch (pair.first) {
                case UserType::STANDARD: cout << "Standard"; break;
                case UserType::PREMIUM: cout << "Premium"; break;
                case UserType::STUDENT: cout << "Student"; break;
                case UserType::FACULTY: cout << "Faculty"; break;
                case UserType::STAFF: cout << "Staff"; break;
                case UserType::GUEST: cout << "Guest"; break;
            }
            cout << ": " << pair.second << "\n";
        }
        
        cout << "\nTotal Book Borrows: " << totalBorrows << "\n";
        cout << "----------------------------------------\n";
    }

    void displayActivityLog(int limit = 20) const {
        cout << "\nAdmin Activity Log for " << username << ":\n";
        cout << "----------------------------------------\n";
        int start = max(0, static_cast<int>(activityLog.size()) - limit);
        for (size_t i = start; i < activityLog.size(); ++i) {
            cout << "- " << activityLog[i] << "\n";
        }
        cout << "----------------------------------------\n";
    }

    string getUsername() const { return username; }
    string getAccessLevel() const { return accessLevel; }
    bool getIsActive() const { return isActive; }

    void updateAccessLevel(const string& newLevel) {
        if (newLevel != "full" && newLevel != "limited" && newLevel != "support") {
            cout << "Invalid access level. Must be 'full', 'limited', or 'support'.\n";
            return;
        }
        accessLevel = newLevel;
        cout << "Access level updated to " << newLevel << ".\n";
    }

    void updatePassword(const string& newPassword) {
        if (!LibraryUtils::validatePassword(newPassword)) {
            cout << "Password must be at least 8 characters with uppercase, lowercase, numbers and special characters.\n";
            return;
        }
        password = newPassword;
        cout << "Password updated successfully.\n";
    }
};

// Library class
class Library {
private:
    vector<unique_ptr<Book>> books;
    unordered_map<string, User> users;
    vector<Admin> admins;
    vector<Transaction> transactions;
    NotificationSystem notificationSystem;
    int nextBookId;
    int nextUserId;
    int nextAdminId;
    int nextTransactionId;
    string libraryName;
    string libraryAddress;
    string establishedDate;
    map<string, int> genrePopularity;
    vector<string> libraryHours;

public:
    Library(string name = "City Central Library", string address = "123 Library St.", 
            string established = "2000-01-01")
        : libraryName(name), libraryAddress(address), establishedDate(established),
          nextBookId(1), nextUserId(1), nextAdminId(1), nextTransactionId(1) {
        // Initialize with default operating hours
        libraryHours = {
            "Monday: 9:00 AM - 6:00 PM",
            "Tuesday: 9:00 AM - 6:00 PM",
            "Wednesday: 9:00 AM - 6:00 PM",
            "Thursday: 9:00 AM - 8:00 PM",
            "Friday: 9:00 AM - 5:00 PM",
            "Saturday: 10:00 AM - 4:00 PM",
            "Sunday: Closed"
        };
        
        // Initialize with some default admins
        admins.emplace_back("admin", "Admin@123", "full", "System Administrator", "admin@library.com");
        admins.emplace_back("librarian", "Lib@1234", "limited", "Head Librarian", "librarian@library.com");
        admins.emplace_back("support", "Support@123", "support", "Support Staff", "support@library.com");
    }

    // Book management methods
    void addBook(unique_ptr<Book> book) {
        if (books.size() >= MAX_BOOKS_IN_LIBRARY) {
            cout << "Cannot add more books. Library capacity reached.\n";
            return;
        }
        book->setId(nextBookId++);
        genrePopularity[book->getGenre()]++;
        books.push_back(move(book));
        cout << "Book added with ID: " << books.back()->getId() << "\n";
        
        // Notify users who might be interested in this genre
        for (auto& userPair : users) {
            for (const auto& favGenre : userPair.second.getFavoriteGenres()) {
                if (books.back()->hasTag(favGenre)) {
                    notificationSystem.sendNotification(
                        userPair.first,
                        "New book added in your favorite genre (" + favGenre + "): " + books.back()->getTitle(),
                        NotificationType::NEW_BOOK_ARRIVAL
                    );
                }
            }
        }
    }

    Book* findBook(int bookId) {
        auto it = find_if(books.begin(), books.end(), 
            [bookId](const unique_ptr<Book>& b) { return b->getId() == bookId; });
        return it != books.end() ? it->get() : nullptr;
    }

    vector<Book*> searchBooks(const string& query) {
        vector<Book*> results;
        string lowerQuery = LibraryUtils::toLower(query);
        
        for (const auto& book : books) {
            if (LibraryUtils::toLower(book->getTitle()).find(lowerQuery) != string::npos ||
                LibraryUtils::toLower(book->getAuthor()).find(lowerQuery) != string::npos ||
                LibraryUtils::toLower(book->getDescription()).find(lowerQuery) != string::npos ||
                book->hasTag(query)) {
                results.push_back(book.get());
            }
        }
        
        return results;
    }

    void displayAllBooks(bool detailed = false) const {
        if (books.empty()) {
            cout << "No books in the library.\n";
            return;
        }
        cout << "\nLibrary Catalog (" << books.size() << " books):\n";
        cout << "========================================\n";
        for (const auto& book : books) {
            if (detailed) {
                book->printDetailedInfo();
            } else {
                book->displayInfo();
                cout << "ID: " << book->getId() << " | ";
                cout << "Status: ";
                switch (book->getStatus()) {
                    case BookStatus::AVAILABLE: cout << "Available"; break;
                    case BookStatus::BORROWED: cout << "Borrowed"; break;
                    case BookStatus::RESERVED: cout << "Reserved"; break;
                    case BookStatus::LOST: cout << "Lost"; break;
                    case BookStatus::DAMAGED: cout << "Damaged"; break;
                    case BookStatus::UNDER_MAINTENANCE: cout << "Under Maintenance"; break;
                }
                cout << "\n----------------------------------------\n";
            }
        }
    }

    void displayBooksByGenre(const string& genre) const {
        string lowerGenre = LibraryUtils::toLower(genre);
        vector<const Book*> genreBooks;
        
        for (const auto& book : books) {
            if (LibraryUtils::toLower(book->getGenre()) == lowerGenre || 
                book->hasTag(genre)) {
                genreBooks.push_back(book.get());
            }
        }
        
        if (genreBooks.empty()) {
            cout << "No books found in genre: " << genre << "\n";
            return;
        }
        
        cout << "\nBooks in Genre \"" << genre << "\" (" << genreBooks.size() << " books):\n";
        cout << "========================================\n";
        for (const auto& book : genreBooks) {
            book->displayInfo();
            cout << "ID: " << book->getId() << "\n";
            cout << "----------------------------------------\n";
        }
    }

    // User management methods
    bool registerUser(string username, string password, string name, string email, UserType type = UserType::STANDARD) {
        if (users.size() >= MAX_USERS) {
            cout << "Cannot register more users. System user limit reached.\n";
            return false;
        }
        if (users.find(username) != users.end()) {
            cout << "Username already exists.\n";
            return false;
        }
        if (!LibraryUtils::validatePassword(password)) {
            cout << "Password must be at least 8 characters with uppercase, lowercase, numbers and special characters.\n";
            return false;
        }
        if (!LibraryUtils::validateEmail(email)) {
            cout << "Invalid email format.\n";
            return false;
        }
        users.emplace(username, User(username, password, name, email, type));
        cout << "User registered successfully with ID: " << nextUserId++ << "\n";
        return true;
    }

    User* authenticateUser(string username, string password) {
        auto it = users.find(username);
        if (it != users.end() && it->second.authenticate(username, password)) {
            return &it->second;
        }
        return nullptr;
    }

    Admin* authenticateAdmin(string username, string password) {
        for (auto& admin : admins) {
            if (admin.authenticate(username, password)) {
                return &admin;
            }
        }
        return nullptr;
    }

    void displayUserInfo(const string& username) const {
        auto it = users.find(username);
        if (it != users.end()) {
            it->second.displayProfile();
        } else {
            cout << "User not found.\n";
        }
    }

    // Transaction operations
    bool borrowBook(User* user, int bookId) {
        if (!user || !user->getIsActive()) {
            cout << "Invalid or inactive user account.\n";
            return false;
        }
        
        Book* book = findBook(bookId);
        if (!book) {
            cout << "Book not found.\n";
            return false;
        }
        
        if (book->getStatus() != BookStatus::AVAILABLE) {
            cout << "Book is currently not available for borrowing.\n";
            if (book->getStatus() == BookStatus::RESERVED) {
                cout << "The book is reserved. You can place a reservation if it becomes available.\n";
            }
            return false;
        }
        
        if (!user->canBorrowMore()) {
            cout << "You've reached your borrow limit (" << user->getBorrowLimit() << " books).\n";
            return false;
        }
        
        if (find(user->getBorrowedBooks().begin(), user->getBorrowedBooks().end(), bookId) 
            != user->getBorrowedBooks().end()) {
            cout << "You've already borrowed this book.\n";
            return false;
        }

        // Calculate due date based on user type
        string dueDate = LibraryUtils::getCurrentDate(); // Should calculate actual due date
        
        book->recordBorrow(user->getUsername());
        user->borrowBook(bookId, dueDate);
        
        // Record transaction
        transactions.emplace_back(nextTransactionId++, user->getUsername(), bookId, "borrow");
        
        cout << "Book \"" << book->getTitle() << "\" borrowed successfully. Due date: " << dueDate << "\n";
        return true;
    }

    bool returnBook(User* user, int bookId) {
        if (!user || !user->getIsActive()) {
            cout << "Invalid or inactive user account.\n";
            return false;
        }
        
        Book* book = findBook(bookId);
        if (!book) {
            cout << "Book not found.\n";
            return false;
        }
        
        if (!user->returnBook(bookId)) {
            return false;
        }
        
        book->recordReturn(user->getUsername());
        
        // Find and update transaction
        for (auto& trans : transactions) {
            if (trans.getBookId() == bookId && trans.getUsername() == user->getUsername() && !trans.getIsReturned()) {
                trans.markReturned();
                break;
            }
        }
        
        cout << "Book \"" << book->getTitle() << "\" returned successfully.\n";
        
        // Check if there are reservations for this book
        if (book->getStatus() == BookStatus::AVAILABLE && book->hasReservations()) {
            notificationSystem.sendNotification(
                book->getNextReservedUser(),
                "The book you reserved (ID: " + to_string(bookId) + ") is now available.",
                NotificationType::RESERVATION_AVAILABLE
            );
        }
        
        return true;
    }

    bool reserveBook(User* user, int bookId) {
        if (!user || !user->getIsActive()) {
            cout << "Invalid or inactive user account.\n";
            return false;
        }
        
        Book* book = findBook(bookId);
        if (!book) {
            cout << "Book not found.\n";
            return false;
        }
        
        if (!book->reserve(user->getUsername())) {
            return false;
        }
        
        user->reserveBook(bookId);
        
        // Record transaction
        transactions.emplace_back(nextTransactionId++, user->getUsername(), bookId, "reserve");
        
        cout << "Book \"" << book->getTitle() << "\" reserved successfully.\n";
        return true;
    }

    bool cancelReservation(User* user, int bookId) {
        if (!user || !user->getIsActive()) {
            cout << "Invalid or inactive user account.\n";
            return false;
        }
        
        Book* book = findBook(bookId);
        if (!book) {
            cout << "Book not found.\n";
            return false;
        }
        
        if (!book->cancelReservation(user->getUsername())) {
            return false;
        }
        
        user->cancelReservation(bookId);
        cout << "Reservation for book \"" << book->getTitle() << "\" cancelled.\n";
        return true;
    }

    void displayBorrowStats() const {
        if (transactions.empty()) {
            cout << "No borrowing statistics available.\n";
            return;
        }
        
        // Convert to vector for sorting
        map<int, int> borrowCounts;
        for (const auto& book : books) {
            borrowCounts[book->getId()] = book->getBorrowCount();
        }
        
        vector<pair<int, int>> stats(borrowCounts.begin(), borrowCounts.end());
        sort(stats.begin(), stats.end(), 
            [](const pair<int, int>& a, const pair<int, int>& b) {
                return a.second > b.second;
            });
        
        cout << "\nMost Borrowed Books (Top 10):\n";
        cout << "========================================\n";
        int limit = min(10, static_cast<int>(stats.size()));
        for (int i = 0; i < limit; i++) {
            Book* book = findBook(stats[i].first);
            if (book) {
                cout << i+1 << ". " << book->getTitle() << " by " << book->getAuthor();
                cout << " - Borrowed " << stats[i].second << " times\n";
            }
        }
    }

    void displayOverdueBooks() const {
        string today = LibraryUtils::getCurrentDate();
        vector<const Transaction*> overdueTransactions;
        
        for (const auto& trans : transactions) {
            if (trans.getType() == "borrow" && !trans.getIsReturned()) {
                int daysOverdue = LibraryUtils::daysBetweenDates(trans.getDueDate(), today);
                if (daysOverdue > 0) {
                    overdueTransactions.push_back(&trans);
                }
            }
        }
        
        if (overdueTransactions.empty()) {
            cout << "No overdue books currently.\n";
            return;
        }
        
        cout << "\nOverdue Books (" << overdueTransactions.size() << "):\n";
        cout << "========================================\n";
        for (const auto& trans : overdueTransactions) {
            Book* book = findBook(trans->getBookId());
            int daysOverdue = LibraryUtils::daysBetweenDates(trans->getDueDate(), today);
            
            cout << "User: " << trans->getUsername() << "\n";
            cout << "Book: " << (book ? book->getTitle() : "Unknown") << " (ID: " << trans->getBookId() << ")\n";
            cout << "Due Date: " << trans->getDueDate() << " (Overdue by " << daysOverdue << " days)\n";
            cout << "Late Fee: $" << fixed << setprecision(2) << daysOverdue * LATE_FEE_PER_DAY << "\n";
            cout << "----------------------------------------\n";
        }
    }

    void displayLibraryInfo() const {
        cout << "\nLibrary Information:\n";
        cout << "========================================\n";
        cout << "Name: " << libraryName << "\n";
        cout << "Address: " << libraryAddress << "\n";
        cout << "Established: " << establishedDate << "\n";
        cout << "Total Books: " << books.size() << "\n";
        cout << "Total Users: " << users.size() << "\n";
        cout << "Total Admins: " << admins.size() << "\n";
        cout << "\nOperating Hours:\n";
        for (const auto& hours : libraryHours) {
            cout << "- " << hours << "\n";
        }
        cout << "========================================\n";
    }

    void checkDueDates() {
        notificationSystem.checkDueDates(transactions);
    }

    void displayPopularGenres() const {
        if (genrePopularity.empty()) {
            cout << "No genre statistics available.\n";
            return;
        }
        
        vector<pair<string, int>> stats(genrePopularity.begin(), genrePopularity.end());
        sort(stats.begin(), stats.end(), 
            [](const pair<string, int>& a, const pair<string, int>& b) {
                return a.second > b.second;
            });
        
        cout << "\nMost Popular Genres (Top 5):\n";
        cout << "========================================\n";
        int limit = min(5, static_cast<int>(stats.size()));
        for (int i = 0; i < limit; i++) {
            cout << i+1 << ". " << stats[i].first << ": " << stats[i].second << " books\n";
        }
    }

    void sendNotificationToUser(const string& username, const string& message, 
                              NotificationType type) {
        if (users.find(username) != users.end()) {
            notificationSystem.sendNotification(username, message, type);
            cout << "Notification sent to " << username << ".\n";
        } else {
            cout << "User not found.\n";
        }
    }

    void displayUserNotifications(const string& username) const {
        if (users.find(username) != users.end()) {
            notificationSystem.displayNotifications(username);
        } else {
            cout << "User not found.\n";
        }
    }
};

// Helper functions for menus
void displayMainMenu() {
    cout << "\n=== Library Management System ===\n";
    cout << "1. User Menu\n";
    cout << "2. Admin Menu\n";
    cout << "3. Display Library Information\n";
    cout << "4. Exit\n";
    cout << "Enter choice: ";
}

void displayUserMenu() {
    cout << "\n=== User Menu ===\n";
    cout << "1. Register\n";
    cout << "2. Login\n";
    cout << "3. View Books\n";
    cout << "4. Search Books\n";
    cout << "5. Return to Main Menu\n";
    cout << "Enter choice: ";
}

void displayLoggedInUserMenu() {
    cout << "\n=== User Dashboard ===\n";
    cout << "1. Borrow Book\n";
    cout << "2. Return Book\n";
    cout << "3. Reserve Book\n";
    cout << "4. Cancel Reservation\n";
    cout << "5. View Borrowed Books\n";
    cout << "6. View Reserved Books\n";
    cout << "7. View Profile\n";
    cout << "8. View Notifications\n";
    cout << "9. Logout\n";
    cout << "Enter choice: ";
}

void displayAdminMenu() {
    cout << "\n=== Admin Menu ===\n";
    cout << "1. Login\n";
    cout << "2. Return to Main Menu\n";
    cout << "Enter choice: ";
}

void displayLoggedInAdminMenu() {
    cout << "\n=== Admin Dashboard ===\n";
    cout << "1. Add Book\n";
    cout << "2. Remove Book\n";
    cout << "3. View All Books\n";
    cout << "4. View System Statistics\n";
    cout << "5. Manage User Accounts\n";
    cout << "6. View Overdue Books\n";
    cout << "7. View Borrowing Stats\n";
    cout << "8. View Popular Genres\n";
    cout << "9. Send Notification\n";
    cout << "10. View Activity Log\n";
    cout << "11. Logout\n";
    cout << "12. Exit\n";
    cout << "Enter choice: ";
}

int main() {
    Library library;

    while (true) {
        displayMainMenu();
        int choice;
        cin >> choice;

        switch (choice) {
            case 1: {
                // User menu logic
                displayUserMenu();
                break;
            }
            case 2: {
                // Admin menu logic
                displayAdminMenu();
                break;
            }
            case 3: {
                library.displayLibraryInfo();
                break;
            }
            case 4:
                cout << "Exiting the system. Goodbye!\n";
                return 0;
            default:
                cout << "Invalid choice. Please try again.\n";
        }
    }

    return 0;
}