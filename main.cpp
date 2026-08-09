#include <iostream>
#include <fstream>
#include <string>
#include "sqlite3.h"

// Helper functions for using command line agrugments
void printUsage()
{
    std::cout << "Usage: ./log_parser -i <input_file> -k <keyword>\n";
    std::cout << "Example: ./log_parser -i system_events.log\n";
}

// argc (argument Count)
// argc (argument Vector)
int main(int argc, char* argv[])
{
    // Configures variables
    std::string inputFile = "";
    std::string searchKeyword = "";
    bool isVerbose = false;

    // Parses command-iine arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-i" && i + 1 < argc)
        {
            inputFile = argv[++i]; // Increments i to grab the next value to be assigned
        }
        else if (arg == "-k" && i + 1 < argc)
        {
            searchKeyword = argv[++i];
        }
        else if (arg == "-v")
        {
            isVerbose = true;
        }
        else
        {
            std::cerr << "Error: Unrecognized argument or missing value for '" << arg << "'\n";
            printUsage();
            return 1;
        }
    }

    // Validates all required arguments were provided
    if (inputFile.empty() || searchKeyword.empty())
    {
        std::cerr << "Error: Missing required arguments. \n";
        printUsage();
        return 1;
    }

    // Opens the database connection
    sqlite3* db;

    // Creates parsed_logs.db'
    int exitCode = sqlite3_open("parsed_logs.db", &db);
    if (exitCode != SQLITE_OK)
    {
        std::cerr << "Error: Cannot open database - " << sqlite3_errmsg(db) << "\n";
        return 1;
    }

    // Creates the table
    const char* createTableSQL =
        "CREATE TABLE IF NOT EXISTS FlaggedEvents ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "TIMESTAMP TEXT, "
        "ERRORLEVEL TEXT, "
        "MESSAGE TEXT);";

    char* dbErrorMessage = nullptr;
    exitCode = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &dbErrorMessage);
    if (exitCode != SQLITE_OK)
    {
        std::cerr << "Table creation error: " << dbErrorMessage << "\n";
        sqlite3_free(dbErrorMessage);
        sqlite3_close(db);
        return 1;
    }

    // Prepares and error checks the SQL insert statement
    const char* insertSQL = "INSERT INTO FlaggedEvents (TIMESTAMP, ERRORLEVEL, MESSAGE) VALUES (?, ?, ?); ";
    sqlite3_stmt* insertStatement;

    exitCode = sqlite3_prepare_v2(db, insertSQL, -1, &insertStatement, nullptr);
    if (exitCode != SQLITE_OK)
    {
        std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        return 1;
    }

    // Processes the Log File
    std::ifstream inFile(inputFile);
    if (!inFile.is_open())
    {
        std::cerr << "Error: Failed to open input file '" << inputFile << "'. Please check the path.\n";
        sqlite3_finalize(insertStatement); // Frees statement memory before exiting
        sqlite3_close(db);
        return 1;
    }

    std::string currentLine;
    int matchCount = 0;
    std::cout << "Scanning logs and writing to SQLite database...\n";

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);


    // Reads line-by-line (memory efficient for large logs)
    while (std::getline(inFile, currentLine))
    {
        if (!currentLine.empty() && currentLine.back() == '\r')
        {
            currentLine.pop_back();
        }

        if (currentLine.find(searchKeyword) != std::string::npos)
        {

            // Sets safe defaults in case of malformeed log line
            std::string timestamp = "UNKNOWN";
            std::string level = "UNKNOWN";
            std::string message = currentLine;

            // Extracts timestamp if long enough
            if (currentLine.length() >= 19)
            {
                timestamp =  currentLine.substr(0, 19);
            }

            // Safely finds brackets
            size_t bracketStart = currentLine.find('[');
            size_t bracketEnd = currentLine.find(']');

            if (bracketStart != std::string::npos && bracketEnd !=
                    std::string::npos && bracketStart < bracketEnd)
            {
                level = currentLine.substr(bracketStart + 1, bracketEnd - bracketStart - 1);

                // Ensures text is between the brackets before slicing
                if (currentLine.length() > bracketEnd + 2)
                {
                    message= currentLine.substr(bracketEnd + 2);
                }
                else
                {
                    message = "";
                }
            }

            // Binds the parsed strings to the SQL statement parameters
            sqlite3_bind_text(insertStatement, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insertStatement, 2, level.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insertStatement, 3, message.c_str(), -1, SQLITE_TRANSIENT);

            // Checks if successful execution of  insert statement
            if (sqlite3_step(insertStatement) != SQLITE_DONE)
            {
                std::cerr << "Warning: Failed to insert record for timestamp" << timestamp << "\n";
            }
            else
            {
                matchCount++;

                if (isVerbose)
                {
                    std::cout << " -> Found: " << currentLine << "\n";
                }

            }

            // Resets the statement for the next loop iteration
            sqlite3_reset(insertStatement);

            sqlite3_clear_bindings(insertStatement);
        }
    }

    exitCode = sqlite3_exec(db, "COMMIT; ", nullptr, nullptr, &dbErrorMessage);
    if (exitCode != SQLITE_OK)
    {
        std::cerr << "Transaction commit error: " << dbErrorMessage << "\n";
        sqlite3_free(dbErrorMessage);
    }

    // Cleans up and summarizes
    inFile.close();                      // Closes the input file
    sqlite3_finalize(insertStatement);  // Destroys the prepared statement
    sqlite3_close(db);                   // Closes the database conectionout

    std::cout << "Database sync complete. Inserted " << matchCount << " record into parsed_logs.db.\n";

    return 0;
}
