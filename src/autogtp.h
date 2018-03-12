#pragma once
#include "gtp_agent.h"
#include <map>



class Game 
{
    GtpProcess proc;
    string m_cmdLine;
    string m_binary;
    string m_workDir;
    string m_timeSettings;
    string m_winner;
    string m_result;
    string m_fileName;

    bool m_resignation{false};
    bool m_blackResigned{false};
    bool m_blackToMove{true};
    int m_passes{0};
    int m_moveNum{0};

public:
    Game(const string& weights,
         const string& opt,
         const string& work_dir,
         const string& binary = string("./leelaz"));

    bool gameStart(const string &min_version);
    bool loadSgf(const string &fileName);
    bool loadTraining(const string &fileName);
    bool saveTraining();
    bool fixSgf(const string& weightFile, bool resignation);
    bool writeSgf();
    string move();
    const string& getFile() const { return m_fileName; }
    bool setMove(const string& m);

    bool checkGameEnd() {
        return (m_resignation ||
                m_passes > 1 ||
                m_moveNum > (19 * 19 * 2));
    }

    bool nextMove();
    int getMovesCount() const { return m_moveNum; }
    void setMovesCount(int moves) {
        m_moveNum = moves;
        m_blackToMove = (moves % 2) == 0;
    }
    const string& getResult() const { return m_result; }

    bool getScore();
    int getWinner();
    const string& getWinnerName() const { return m_winner; }

    bool dumpTraining();
    bool dumpDebug();
    void gameQuit();



    enum {
        BLACK = 0,
        WHITE = 1,
    };

private:
    bool sendGtpCommand(const string& cmd);
    void checkVersion(const string &min_version);
};


class Result {
public:
    enum Type {
        File = 0,
        Win,
        Loss,
        Waited,
        StoreMatch,
        StoreSelfPlayed,
        Error
    };
    Result() = default;
    Result(int t, map<string,string> n = {}) { m_type = t, m_parameters = n; }
    ~Result() = default;
    void type(int t) { m_type = t; }
    int type() { return m_type; }
    void add(const string &name, const string &value) { m_parameters[name] = value; }
    map<string,string> parameters() { return m_parameters; }
    void clear() { m_parameters.clear(); }
private:
    int m_type;
    map<string,string> m_parameters;
};

class Order {
public:
    enum {
        Error = 0,
        Production,
        Validation,
        Wait,
        RestoreMatch,
        RestoreSelfPlayed
    };
    Order() = default;
    Order(int t, map<string,string> p = {}) { m_type = t; m_parameters = p; }
    Order(const Order &o) { m_type = o.m_type; m_parameters = o.m_parameters; }
    Order &operator=(const Order &o) { m_type = o.m_type; m_parameters = o.m_parameters; return *this; }
    ~Order() = default;
    void type(int t) { m_type = t; }
    int type() const { return m_type; }
    map<string,string> parameters() const { return m_parameters; }
    void parameters(const map<string,string> &l) { m_parameters = l; }
    void add(const string &key, const string &value) { m_parameters[key] = value; }
    bool isValid() { return (m_type > Error && m_type <= RestoreSelfPlayed); }
    void save(const string &file);
    void load(const string &file);

private:
    int m_type;
    map<string,string> m_parameters;
};

void executeJob(const Order &o,
    const string& workDir,
    const string& gpu = "");

Result executeProductionJob(
    const Order &o,
    const string& workDir,
    const string& gpu = ""
);


