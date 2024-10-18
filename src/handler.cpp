#include <filesystem>
#include "glzt.cpp"
#include "huffman.cpp"
#pragma once

class Konsole
{
private:
    enum FLAGS : uint16_t
    {
        VERBOSE = 0b1,
        START = 0b10,
        END = 0b100,
        EDIT = 0b1000,
        HELP = 0b10000,
        PRINT = 0b100000,
    };

    uint16_t flags;
    std::string Inputpath;
    std::string Outputpath;
    Gleitzeit *gleitzeit;
    int summe;

    void log(const std::string &) const;
    void genFlags(const std::vector<std::string> &args);
    void init();
    bool correctTimes();
    bool addBegin();
    bool addEnd();
    void timeInfo();
    void printHelp();


public:
    Konsole(const std::vector<std::string> &args);
    ~Konsole();
    void start();
    static bool userAgreed()
    {
        char input = 0;
        std::cin >> input;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return (std::tolower(input) == 'y' || std::tolower(input) == 'j');
    }
};

Konsole::Konsole(const std::vector<std::string> &args) : gleitzeit(nullptr), summe(0)
{
    genFlags(args);

    if (flags & START && flags & END)
    {
        std::cerr << "\n!!!!!Du versuchst Arbeitsstart und ArbeiworkEnde gleichzeitig einzutragen...!!!!!\nAbbruch\n";
        std::abort();
    }
}

Konsole::~Konsole()
{
    if (gleitzeit == nullptr)
        delete gleitzeit;
}

void Konsole::log(const std::string &message) const
{
    if (flags & VERBOSE)
        std::cout << message << "\n";
}

void Konsole::genFlags(const std::vector<std::string> &args)
{
    for (auto it = args.begin()+1; it != args.end(); ++it)
    {
        if (*it == "-v")
            flags |= VERBOSE;

        else if (*it == "start")
            flags |= START;

        else if (*it == "end")
            flags |= END;

        else if (*it == "edit")
            flags |= EDIT;

        else if (*it == "print")
            flags |= PRINT;

        else if (*it == "-i" && (it + 1 != args.end()) && std::filesystem::exists(*(it + 1)))
            Inputpath = *(++it);

        else if (*it == "-o" && (it + 1 != args.end()))
            Outputpath = *(++it);

        else 
            flags |= HELP;
    }
}

void Konsole::init()
{
    if (!Inputpath.empty() && std::filesystem::exists(Inputpath))
    {
        gleitzeit = new Gleitzeit(Inputpath);
    }
    else
    {
        if (std::filesystem::exists("data.bin"))
        {
            log("Lese Original ein");
            gleitzeit = new Gleitzeit("data.bin");
        }
        else
        {
            log("Das Programm konnte keine Dateien finden! Erstelle eine neue Gleitzeittabelle...");
            gleitzeit = new Gleitzeit();
        }
    }
}

bool Konsole::correctTimes()
{
    bool changed = false;
    log("Berechne Summen....");
    std::vector<Gleitzeit::UpdatedTimes> *times = new std::vector<Gleitzeit::UpdatedTimes>;
    int sum = gleitzeit->correct(times);

    if (!times->empty())
    {
        changed = true;
        std::cout << std::string(40, '_') << "\n";
        std::cout << "Folgende Zeiten wurden angepasst:\n";
        for (auto it = times->begin(); it != times->end(); it++)
        {
            std::cout << "--------- KW" << it->week << "!: ---------"
                      << "\nAlter Wert: " << it->oldVal
                      << "\nNeuer Wert: " << it->newVal << "\n";
        }
        std::cout << std::string(40, '_') << "\n";
    }
    delete times;
    return changed;
}

void Konsole::start()
{
    bool needToSave = false;

    if(flags & HELP)
    {
        printHelp();
        return ;
    }

    init();

    if (gleitzeit == nullptr)
        return;

    log("Berechne Summen der einzelnen Wochen und überprüfe sie auf Fehler...");
    needToSave |= correctTimes();

    if (flags & VERBOSE)
        timeInfo();
    log("Aktuell verfügbare gleitzeit: " + std::to_string(gleitzeit->getFlexiTime()) + " min");

    if (flags & PRINT)
        std::cout << gleitzeit->getMarkdown();

    if (flags & START)
        needToSave |= addBegin();
    else if (flags & END)
        needToSave |= addEnd();

    if (!Inputpath.empty())
        needToSave = true;

    if (needToSave || !Outputpath.empty())
    {
        std::cout << "Speichern?(y/N): ";
        if (userAgreed())
            gleitzeit->save(Outputpath);
    }
}

bool Konsole::addBegin()
{
    bool change = true;
    // get kalendar number
    if (gleitzeit->isNewWeek() == true)
    {
        std::cout << "Es wird eine Neue Woche angelegt." << std::endl;
        std::cout << "Wochennummer: " << gleitzeit->getWeekNumber() << "\n";
        gleitzeit->addWeek();
    }

    // get date
    int workBegin = gleitzeit->getWorkBegin();
    int workLimit = gleitzeit->getWorkLimits().first;

    if (!gleitzeit->isNewDay())
    {
        std::cout << "Es gibt schon einen Eintrag für diesen Tag. Manuelle Anpassung notwendig" << std::endl;
        return false;
    }

    else if (gleitzeit->isNewDay() && workBegin < 0)
    {
        std::cout << std::string(40, '-') << "\n";
        std::cout << "Sie wollen " << workBegin << " min negative Gleitzeit eintragen... Ist das Okay?(y/N)" << std::endl;
        if (userAgreed())
        {
            gleitzeit->addEntry(workBegin);
            std::cout << "Es wurden " << workBegin << " Minuten von der Gleitzeit abgezogen\n";
        }
        else
        {
            std::cout << "Es wurde nichts eingetragen...\n";
            change = false;
        }
        std::cout << std::string(40, '-') << "\n";
    }

    else if (workBegin < workLimit)
    {
        std::cout << "Es wird ein Neuer ein Eintrag für diesen Tag Angelegt!\n";
        std::cout << workBegin << " Minuten bis zum Arbeitsbeginn\n";
        gleitzeit->addEntry(workBegin);
    }

    else
    {
        std::cout << "Die Gleitzeit die du Eintragen möchtest liegt über dem Limit (" << workLimit << " min).\n";
        change = false;
    }
    correctTimes();
    return change;
}

bool Konsole::addEnd()
{
    // get kalendar number
    bool change = true;
    if (gleitzeit->isNewWeek() == true)
    {
        std::cout << "Es wird eine Neue Woche angelegt." << std::endl;
        gleitzeit->addWeek();
    }

    // get date
    int workEnd = gleitzeit->getWorkEnd();
    int workLimit = gleitzeit->getWorkLimits().second;

    if (!gleitzeit->isNewDay())
    {
        std::cout << std::string(40, '-') << "\n";
        std::cout << "Es gibt schon einen Eintrag für diesen Tag.\nSoll die Zeit verrechnet werden?(y/N) " << std::endl;
        if (userAgreed())
        {
            if (workEnd < 0)
            {
                std::cout << "Sie wollen " << workEnd << " min negative Gleitzeit eintragen... Ist das Okay?(y/N)" << std::endl;
                if (userAgreed())
                {
                    gleitzeit->addEntry(workEnd);
                    std::cout << "Es wurden " << workEnd << " Minuten von der Gleitzeit abgezogen\n";
                }
                else
                {
                    std::cout << "Es wurde nichts eingetragen...\n";
                    change = false;
                }
            }
            else if (workEnd < workLimit)
            {
                std::cout << "Es wird ein Neuer ein Trag für diesen Tag Angelegt!/n";
                std::cout << workEnd << " Minuten seit ArbeiworkEnde\n";
                gleitzeit->addEntry(workEnd);
            }
            else
            {
                std::cout << "Die Gleitzeit die du Eintragen möchtest liegt über dem Limit (" << workLimit << " min).\n";
            }
        }
        else
        {
            std::cout << "Es wurde nichts eingetragen...\n";
            change = false;
        }
        std::cout << std::string(40, '-') << "\n";
    }

    else if (gleitzeit->isNewDay() && workEnd < 0)
    {
        std::cout << std::string(40, '-') << "\n";
        std::cout << "Sie wollen " << workEnd << " min negative Gleitzeit eintragen... Ist das Okay?(y/N)" << std::endl;
        if (userAgreed())
        {
            gleitzeit->addEntry(workEnd);
            std::cout << "Es wurden " << workEnd << " Minuten von der Gleitzeit abgezogen\n";
        }

        else
        {
            std::cout << "Es wurde nichts eingetragen...\n";
            change = false;
        }
        std::cout << std::string(40, '-') << "\n";
    }

    else if (!gleitzeit->isNewDay() && workEnd < 0)
    {
        std::cout << std::string(40, '-') << "\n";
        std::cout << "Sie wollen " << workEnd << " min negative Gleitzeit eintragen... Ist das Okay?(y/N)" << std::endl;
        if (userAgreed())
        {
            gleitzeit->addEntry(workEnd);
            std::cout << "Es wurden " << workEnd << " Minuten von der Gleitzeit abgezogen\n";
        }
        else
        {
            std::cout << "Es wurde nichts eingetragen...\n";
        }
        std::cout << std::string(40, '-') << "\n";
    }

    else if (workEnd < workLimit)
    {
        std::cout << "Es wird ein Neuer ein Trag für diesen Tag Angelegt!/n";
        std::cout << workEnd << " Minuten seit ArbeiworkEnde\n";
        gleitzeit->addEntry(workEnd);
    }

    else
    {
        std::cout << "Die Gleitzeit die du Eintragen möchtest liegt über dem Limit (" << workLimit << " min).\n";
    }
    correctTimes();
    return change;
}

void Konsole::timeInfo()
{
    int begin = gleitzeit->getWorkBegin();
    int end = gleitzeit->getWorkEnd();
    tm workBegin = gleitzeit->getDayBegin();
    tm workEnd = gleitzeit->getDayEnd();
    tm now = gleitzeit->now();
    // print distance
    std::cout << begin << " Minuten bis zum Arbeitsbeginn\n";
    std::cout << end << " Minuten seit Arbeitsende\n";

    std::cout << "Arbeitsbegin: " << std::put_time(&workBegin, "%d.%m.%Y %H:%M:%S") << std::endl;
    std::cout << "Arbeitsende : " << std::put_time(&workEnd, "%d.%m.%Y %H:%M:%S") << std::endl;
    std::cout << std::put_time(&now, "Kalenderwoche: %W KW \nTag:           %d.%m \nUhrzeit:       %H:%M Uhr") << std::endl;
}

void Konsole::printHelp()
{
    std::cout << std::string(30,'=') << "\n===FlexiStamp==\nDesc:A Program to easy generate your Flexitimetable\nBy: Meik Mittwoch 2024\nUsage:\n";
    std::cout << "\tflexistamp -h             --Getting This Help Menu\n";
    std::cout << "\tflexistamp -i [filename]  --Read other File than data.bin\n";
    std::cout << "\tflexistamp -o [filename]  --Export File under Filename as a Markdown\n";
    std::cout << "\tflexistamp print          --Prints the Markdown in the Console\n";
    std::cout << "\tflexistamp start          --adds the Flexitime BEFORE your Workbegin to your Flextimeshedule\n";
    std::cout << "\tflexistamp end            --adds the Flexitime AFTER  your Workbegin to your Flextimeshedule\n";


}
