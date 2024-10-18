#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <limits>
#include "huffman.cpp"
#pragma once

class Gleitzeit
{
private:
    struct glz
    {
        std::string week;
        std::vector<std::pair<std::string, int>> data;
        int sum = std::numeric_limits<int>::max();
    };

    struct timeStamp
    {
        std::time_t timeNow;
        std::tm timeNowTm;
        std::tm WorkStartTime;
        std::tm WorkEndTime;
        int begin;
        int end;
        int weekNumber;
        std::string dayAsString;
        int limitBegin;
        int limitEnd;

        timeStamp(int limitBegin = 45, int limitEnd = 120) : limitBegin(limitBegin), limitEnd(limitEnd)
        {
            // set Timestamp
            timeNow = std::time(nullptr);
            timeNowTm = *std::localtime(&timeNow);
            WorkStartTime = {0, 30, 7, timeNowTm.tm_mday, timeNowTm.tm_mon, timeNowTm.tm_year};
            WorkEndTime = {0, 15, 15, timeNowTm.tm_mday, timeNowTm.tm_mon, timeNowTm.tm_year};

            // calc distance
            begin = std::ceil(std::difftime(std::mktime(&WorkStartTime), timeNow) / 60);
            end = std::ceil(std::difftime(timeNow, std::mktime(&WorkEndTime)) / 60);

            // get Weeknumber
            std::stringstream formatter;
            formatter << std::put_time(&timeNowTm, "%W");
            weekNumber = std::stoi(formatter.str());

            // get Day as string (dd.mm)
            dayAsString = (std::to_string(timeNowTm.tm_mday) + "." + std::to_string(timeNowTm.tm_mon + 1));
        }
    };

    std::vector<glz> container;
    std::stringstream *stream;
    void generate();
    int getWeekNumber(const std::tm *);
    void init(std::istream &);
    timeStamp t;


public:
    struct UpdatedTimes
    {
        std::string week;
        int oldVal;
        int newVal;
    };

    Gleitzeit(std::string);
    Gleitzeit();
    ~Gleitzeit();
    void save(std::string);
    int correct(std::vector<UpdatedTimes> *);
    void writeMarkdown(const std::string);
    std::string getMarkdown();

    bool isNewWeek() const { return (std::to_string(t.weekNumber) != container.back().week); }
    bool isNewDay() const { return (container.back().data.empty() ? true : (t.dayAsString != container.back().data.back().first)); }
    void addEntry(const int time) { container.back().data.push_back({t.dayAsString, time}); }
    void addWeek() { container.push_back(glz{std::to_string(t.weekNumber)}); }
    int getWeekNumber() const { return t.weekNumber; }
    int getWorkBegin() const { return t.begin; }
    int getWorkEnd() const { return t.end; }
    int getFlexiTime() const { return container.back().sum; }
    tm getDayBegin() const { return t.WorkStartTime; }
    tm getDayEnd() const { return t.WorkEndTime; }
    tm now() const { return t.timeNowTm; }
    std::pair<int, int> getWorkLimits() const { return {t.limitBegin, t.limitEnd}; }
};

Gleitzeit::Gleitzeit(std::string adress)
{
    stream = nullptr;
    std::ifstream file = std::ifstream(adress);

    if (!file.is_open())
    {
        std::cerr << "File not Fount\n"
                    << "Path:" << adress << std::endl;
        std::abort();
    }

    if(adress.find(".md") != std::string::npos)
    {
        init(file);
    }
    else if(adress.find(".bin") != std::string::npos)
    {
        stream = new std::stringstream;
        Huffman *huff = new Huffman;
        huff->decompress(file, *stream);
        delete huff;
        init(*stream);
    }
    file.close();
}

Gleitzeit::Gleitzeit()
{
    stream = new std::stringstream;

    // if(std::filesystem::exists("data.bin"))
    // {
    //     Huffman *huff = new Huffman;
    //     std::ifstream BinaryInput("data.bin", std::ifstream::binary);
    //     huff->decompress(BinaryInput, *stream);
    //     BinaryInput.close();
    //     delete huff;
    // }
    // Markdown.close();
    // init("temp.md");
    init(*stream);
}

Gleitzeit::~Gleitzeit()
{
    if (stream != nullptr)
        delete stream;
}

void Gleitzeit::save(std::string adress)
{
    correct(nullptr);
    if (!adress.empty())
        writeMarkdown(adress);
    // std::ifstream toSave(adress, std::ifstream::binary);
    std::ofstream dataOut("data.bin");
    Huffman *huff = new Huffman;
    if (stream != nullptr)
    {
        huff->compress(*stream, dataOut);
        delete huff;
        std::cout << "Speichern erfolgreich abgeschlossen" << "\n";
    }
}

void Gleitzeit::generate()
{
    if (stream != nullptr)
        delete stream;

    stream = new std::stringstream;
    *stream << "# Gleitzeit:\n";

    for (auto &week : container)
    {
        *stream << "## KW " << week.week << "\n";
        *stream << "|`Art` |`Datum` |`Zeit`         |\n|------|:------:|--------------:|\n";

        for (auto &data : week.data)
        {
            *stream << "|" << std::setw(6) << (data.second >= 0 ? "Aufbau" : "Abbau") << "|" << std::setw(8) << data.first << "|" << std::setw(15) << (data.second >= 0 ? "+ " : "") + std::to_string(data.second) << "|\n";
        }

        *stream << "_________________________________\n";

        *stream << "###\t\t\t\t\t\t\t" << (week.sum >= 0 ? "+ " : "  ") << week.sum << "\n";
    }
    *stream << "\n\n";
}

void Gleitzeit::init(std::istream &file)
{
    std::string buffer;

    while (file.good())
    {
        std::getline(file, buffer);
        int pos = 0;
        if (buffer.empty())
            break;
        do
        {
            // clear All unnecessarcy
            pos = buffer.find_first_of("#!-_: `ABCDEFGHIJLKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
            if (pos != std::string::npos)
                buffer.erase(pos, 1);
        } while (pos != std::string::npos);

        if (buffer.empty() )
            continue;

        if (buffer.find("|") == std::string::npos)
        {
            // find week
            if (buffer.find("+") == std::string::npos)
            {
                if (container.empty() || container.back().sum != std::numeric_limits<int>::max())
                {
                    container.push_back(glz());
                    container.back().week = buffer;
                }
                // if its a negative sum of week
                else
                {
                    container.back().sum = std::stoi(buffer.substr(1)) * -1;
                }
            }
            // find sum of Week
            else
            {
                container.back().sum = std::stoi(buffer.substr(1));
            }
        }

        else
        {
            // empty line
            if (buffer.find_first_not_of("|") == std::string::npos)
            {
                continue;
            }
            // day found
            else
            {
                buffer.erase(0, 2);
                std::string date = buffer.substr(0, buffer.find("|"));

                buffer.erase(0, buffer.find("|") + 1);
                if (buffer.at(0) == '+')
                    container.back().data.push_back({date, std::stoi(buffer.substr(1))});
                else
                    container.back().data.push_back({date, std::stoi(buffer) * -1});
            }
        }
    }

    if(container.empty())
    {
        addWeek();
        addEntry(0);
        correct(nullptr);
        return;
    }
}

int Gleitzeit::correct(std::vector<UpdatedTimes> *data = nullptr)
{
    int sum = 0;
    for (auto &week : container)
    {
        int calc = 0;

        for (auto &add : week.data)
            calc += add.second;

        sum += calc;
        if (week.sum != sum)
        {
            if (data != nullptr)
                data->push_back({week.week, week.sum, sum});

            week.sum = sum;
        }
    }
    generate();
    return sum;
}

void Gleitzeit::writeMarkdown(const std::string filename)
{
    generate();
    std::ofstream out(filename);
    out << (*stream).str();
    out.close();
}

int Gleitzeit::getWeekNumber(const std::tm *timeStamp)
{
    std::stringstream formatter;
    formatter << std::put_time(timeStamp, "%W");
    return std::stoi(formatter.str());
}

std::string Gleitzeit::getMarkdown()
{
    generate();
    return (*stream).str();
}