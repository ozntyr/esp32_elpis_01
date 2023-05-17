
#pragma once

#include <SPIFFS.h>

String known_ssids[20];
String known_passs[20];
int known_len = 3;

void list_Dir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("− failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" − not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                list_Dir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void read_File(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("− failed to open file for reading");
        return;
    }

    Serial.println("− read from file:");
    while (file.available())
    {
        Serial.write(file.read());
    }
}

void write_File(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("− failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("− file written");
    }
    else
    {
        Serial.println("− frite failed");
    }
}

void append_File(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("− failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("− message appended");
    }
    else
    {
        Serial.println("− append failed");
    }
}

void rename_File(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("− file renamed");
    }
    else
    {
        Serial.println("− rename failed");
    }
}

void delete_File(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path))
    {
        Serial.println("− file deleted");
    }
    else
    {
        Serial.println("− delete failed");
    }
}

void read_SsidPasswords()
{
    // Open file for reading
    File file = SPIFFS.open("/ssid_passwords.txt", "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }
    else
    {
        Serial.println("read: /ssid_passwords.txt");
        known_len = 0;
    }

    // Read file contents and store in global variables
    String line;
    while (file.available())
    {
        line = file.readStringUntil('\n');
        Serial.println(line);
        line.trim();
        if (!line.isEmpty())
        {
            int colonIndex = line.indexOf(':');
            Serial.println(colonIndex);
            if (colonIndex != -1)
            {
                known_ssids[known_len] = line.substring(0, colonIndex);
                known_passs[known_len] = line.substring(colonIndex + 1);
                Serial.println(known_ssids[known_len]);
                Serial.println(known_passs[known_len]);
                known_len++;
            }
        }
    }
    file.close();
}

String load_Options()
{
    // Load options from file and inject into HTML
    File file = SPIFFS.open("/options.txt", "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return "";
    }
    String optionList = "";
    Serial.println("optionList: ");

    while (file.available())
    {
        String option = file.readStringUntil('\n');

        Serial.println("opt: ");
        Serial.println(option);

        option.trim();
        if (!option.isEmpty())
        {
            optionList += "<option>" + option + "</option>";
        }
    }
    file.close();

    // Load HTML file from SPIFFS
    file = SPIFFS.open("/index.html", "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return "";
    }
    String html = file.readString();
    file.close();

    // Inject options into HTML
    html.replace("{OPTIONS}", optionList);

    // Set HTML as global variable
    return html;
}

void setup_Spiffs()
{
    // Initialize SPIFFS file system
    if (!SPIFFS.begin(true))
    {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }
    else
    {
        Serial.println("Mounted SPIFFS");
    }

    list_Dir(SPIFFS, "/", 0);

    Serial.println("reading ssids");

    // Read SSIDs and passwords from file
    read_SsidPasswords();

    // Load options and inject into HTML
    load_Options();
}
