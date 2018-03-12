#include "autogtp.h"
#include <iostream>
#include <fstream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/replace.hpp>


static void error_exit(const string& str) {
    cerr << str << endl;
    throw std::runtime_error(str);
}

static vector<int> get_ver_list(const string& ver) {

    std::stringstream ss(ver);
    std::string item;
    std::vector<int> ver_list;
    while (std::getline(ss, item, '.')) {
        ver_list.push_back(stoi(item));
    }
    while (ver_list.size() < 3) ver_list.push_back(0);

    return ver_list;
}

static vector<string> split_str(const string& name, char delim) {

    std::stringstream ss(name);
    std::string item;
    std::vector<std::string> names;
    while (std::getline(ss, item, delim)) {
        names.push_back(item);
    }

    return names;
}


Game::Game(const string& weights, const string& opt, const string& work_dir, const string& binary) :
    m_binary(binary),
    m_workDir(work_dir),
    m_timeSettings("time_settings 0 1 0"),
    m_resignation(false),
    m_blackToMove(true),
    m_blackResigned(false),
    m_passes(0),
    m_moveNum(0)
{
#ifdef WIN32
    m_binary.append(".exe");
#endif
    m_cmdLine = m_binary + " " + opt + " " + weights;
    boost::uuids::random_generator gen;
    boost::uuids::uuid u = gen();
    m_fileName = boost::replace_all_copy(boost::uuids::to_string(u), "-", "");

    proc.onStderr = [](const string& line) { std::cerr << line << std::endl;  };
}

bool Game::sendGtpCommand(const string& cmd) {
    bool ok;
    GtpState::send_command_sync(proc, cmd, ok);
    return ok;
}

bool Game::setMove(const string& m) {

    if (!sendGtpCommand(m)) {
            return false;
    }
    m_moveNum++;
    if (m.find("pass") != string::npos) {
            m_passes++;
    } else if (m.find("resign") != string::npos) {
            m_resignation = true;
            m_blackResigned = (m.find("black") != string::npos);
    } else {
            m_passes = 0;
    }
    m_blackToMove = !m_blackToMove;
    return true;
}


bool Game::fixSgf(const string& weightFile, bool resignation) {
    
    ifstream sgfFile(m_fileName + ".sgf");
    if (!sgfFile) {
        return false;
    }

    string sgfData((std::istreambuf_iterator<char>(sgfFile)),  
                 std::istreambuf_iterator<char>());  

     string playerName("PB[Leela Zero ");
     auto le_pos = sgfData.find("PB[Leela Zero ");
     if (le_pos != string::npos) {
         auto eof = sgfData.find("]", le_pos + 14);
         auto end_pos = sgfData.find(" ", le_pos + 14);
         if (end_pos != string::npos && end_pos < eof)
            playerName = sgfData.substr(le_pos, (end_pos-le_pos+1));
    }
    playerName = "PW" + playerName.substr(2);
    playerName += weightFile.substr(0,8);
    playerName += "]";

    boost::replace_all(sgfData, "PW[Human]", playerName);

    if(resignation) {
        auto pos = sgfData.find("RE[B+");
        if (pos == string::npos)
            pos = sgfData.find("RE[W+");
        if (pos != string::npos) {
            auto eof = sgfData.find("]", pos);
            if (eof != string::npos) {
                auto oldResult = sgfData.substr(pos, eof-pos+1);
                boost::replace_all(sgfData, oldResult, "RE[B+Resign]");
            }
        }
        boost::replace_all(sgfData, ";W[tt])", ")");
    }

    sgfFile.close();
    ofstream ofs(m_fileName + ".sgf");
    if(ofs) {
        ofs << sgfData;
        ofs.close();
    }
    
    return true;
}


bool Game::gameStart(const string &min_version) {

    proc.execute(m_cmdLine, m_workDir, 50);

    if (!proc.isReady()) {
        error_exit("cannot start leelaz.");
        return false;
    }

    // This either succeeds or we exit immediately, so no need to
    // check any return values.
    checkVersion(min_version);
    cout << "Engine has started." << endl;
    GtpState::send_command_sync(proc, m_timeSettings);
    cout << "Infinite thinking time set." << endl;
    return true;
}


void Game::checkVersion(const string &min_version) {

    auto min_version_list = get_ver_list(min_version);
    auto ver_list = get_ver_list(proc.version());

    if (!proc.isReady())
        error_exit("engine not ready.");

    int versionCount = (ver_list[0] - min_version_list[0]) * 10000;
    versionCount += (ver_list[1] - min_version_list[1]) * 100;
    versionCount += ver_list[2] - min_version_list[2];
    if (versionCount < 0) {
        error_exit(string("Leela version is too old, saw ") + proc.version() + 
                                    string(" but expected ") + min_version +
                                    string("\nCheck https://github.com/gcp/leela-zero for updates."));
    }
}

string Game::move() {
    m_moveNum++;
    bool  ok;
    auto m_moveDone = GtpState::send_command_sync(proc, m_blackToMove ? "genmove b" : "genmove w", ok);
    if (!ok) {
        error_exit("error while genmove.");
    }

    cout << m_moveNum << " (";
    cout << (m_blackToMove ? "B " : "W ") << m_moveDone << ") " << std::flush;

    if (m_moveDone =="pass") {
        m_passes++;
    } else if (m_moveDone == "resign") {
        m_resignation = true;
        m_blackResigned = m_blackToMove;
    } else {
        m_passes = 0;
    }

    return m_moveDone;
}

bool Game::nextMove() {
    if(checkGameEnd()) {
        return false;
    }
    m_blackToMove = !m_blackToMove;
    return true;
}

bool Game::writeSgf() {
    return sendGtpCommand("printsgf " + m_fileName + ".sgf");
}

bool Game::loadTraining(const string &fileName) {
    cout << "Loading " << fileName + ".train" << endl;
    return sendGtpCommand("load_training " + fileName + ".train");

}

bool Game::saveTraining() {
     cout << "Saving " << m_fileName + ".train" << endl;
     return sendGtpCommand("save_training " + m_fileName + ".train");
}


bool Game::loadSgf(const string &fileName) {
    cout << "Loading " << fileName + ".sgf" << endl;
    return sendGtpCommand("loadsgf " + fileName + ".sgf");
}


bool Game::dumpTraining() {
    return sendGtpCommand("dump_training " + m_winner + " " + m_fileName + ".txt");
}

bool Game::dumpDebug() {
    return sendGtpCommand("dump_debug " + m_fileName + ".debug.txt");
}

void Game::gameQuit() {
    proc.send_command("quit");
    GtpState::wait_quit(proc);
}

int Game::getWinner() {
    if(m_winner == "white")
        return Game::WHITE;
    else
        return Game::BLACK;
}

bool Game::getScore() {

    if(m_resignation) {
        if (m_blackResigned) {
                m_winner = "white";
                m_result = "W+Resign ";
                cout << "Score: " << m_result << endl;
        } else {
                m_winner = "black";
                m_result = "B+Resign ";
                cout << "Score: " << m_result << endl;
        }
    } else{
        bool ok;
        m_result = GtpState::send_command_sync(proc, "final_score", ok);
        if (!ok)
                error_exit("error execute final_score");

        if (m_result[0] == 'W') {
                m_winner = "white";
        } else if (m_result[0] == 'B') {
                m_winner = "black";
        }
  
        cout << "Score: " << m_result;
    }
    if (m_winner.empty()) {
        cout << "No winner found" << endl;
        return false;
    }
    cout << "Winner: " << m_winner << endl;
    return true;
}

///////////////////////////////////////////////////////////////
// Order
void Order::save(const string &file) {
    ofstream f(file);
    if (!f) {
        return;
    }
    f << m_type << endl;
    f << m_parameters.size() << endl;
    for(auto& pair : m_parameters)
    {
        f << pair.first << " " << pair.second << endl;
    }
    f.close();       
}

void Order::load(const string &file) {
    ifstream f(file);
    if (!f) {
        return;
    }

    f >>  m_type;
    int count;
    f >> count;
    string key;
    for(int i = 0; i < count; i++) {
        f >> key;
        if(key == "options") {
            std::getline(f, m_parameters[key]);
        } else {
            f >> m_parameters[key];
        }
    }
    f.close();
}


///////////////////////////////////////////////////////////////
// JOB

Result executeProductionJob(
    const Order &o,
    const string& workDir,
    const string& gpu
) {

    string sgf;
    int moves = 0;

    auto option = " " + o.parameters()["options"] + gpu + " -g -q -w ";
    auto leelazMinVersion = o.parameters()["leelazVer"];

    auto network = o.parameters()["network"];
    bool debug = o.parameters()["debug"] == "true";

    if(o.type() == Order::RestoreSelfPlayed) {
        sgf = o.parameters()["sgf"];
        moves = stoi(o.parameters()["moves"]);
    }

    Result res(Result::Error);
    Game game(network, option, workDir);
    if (!game.gameStart(leelazMinVersion)) {
        return res;
    }
    if(!sgf.empty()) {
        game.loadSgf(sgf);
        game.loadTraining(sgf);
        game.setMovesCount(moves);
        //QFile::remove(m_sgf + ".sgf");
        //QFile::remove(m_sgf + ".train");
    }
    do {
        auto mtext = game.move();
        if (mtext.empty() || mtext[0] == '?') {
            return res;
        }
    }while(game.nextMove());

    cout << "Game has ended." << endl;
    if (game.getScore()) {
        game.writeSgf();
        game.fixSgf(network, false);
        game.dumpTraining();
        if (debug) {
            game.dumpDebug();
        }
    }
    res.type(Result::File);
    res.add("file", game.getFile());
    res.add("winner", game.getWinnerName());
    res.add("moves", std::to_string(game.getMovesCount()));

    game.gameQuit();
    return res;
}



Result executeValidationJob(
    const Order &o,
    const string& workDir,
    const string& gpu
) {
    string m_sgfFirst, m_sgfSecond;
    int m_moves = 0;

    auto m_option = " " + o.parameters()["options"] + gpu + " -g -q -w ";
    auto m_leelazMinVersion = o.parameters()["leelazVer"];

    auto m_firstNet = o.parameters()["firstNet"];
    auto m_secondNet = o.parameters()["secondNet"];
    if(o.type() == Order::RestoreMatch) {
        m_sgfFirst = o.parameters()["sgfFirst"];
        m_sgfSecond = o.parameters()["sgfSecond"];
        m_moves = stoi(o.parameters()["moves"]);
    }

    Result res(Result::Error);
    Game first(m_firstNet,  m_option, workDir);
    if (!first.gameStart(m_leelazMinVersion)) {
        return res;
    }
    if(!m_sgfFirst.empty()) {
        first.loadSgf(m_sgfFirst);
        first.setMovesCount(m_moves);
        //QFile::remove(m_sgfFirst + ".sgf");
    }
    Game second(m_secondNet, m_option, workDir);
    if (!second.gameStart(m_leelazMinVersion)) {
        return res;
    }
    if(!m_sgfSecond.empty()) {
        second.loadSgf(m_sgfSecond);
        second.setMovesCount(m_moves);
        //QFile::remove(m_sgfSecond + ".sgf");
    }

    string wmove = "play white ";
    string bmove = "play black ";

    do {
        auto mtext = first.move();
        if (mtext.empty() || mtext[0] == '?') {
            return res;
        }
        // m_boss->incMoves();
        if (first.checkGameEnd()) {
            break;
        }
        second.setMove(bmove + mtext);
        mtext = second.move();
        if (mtext.empty() || mtext[0] == '?') {
            return res;
        }
        //m_boss->incMoves();
        first.setMove(wmove + mtext);
        second.nextMove();
    } while (first.nextMove());

    res.add("moves", to_string(first.getMovesCount()));
    cout << "Game has ended." << endl;
    if (first.getScore()) {
        res.add("score", first.getResult());
        res.add("winner", first.getWinnerName());
        first.writeSgf();
        first.fixSgf(m_secondNet, (res.parameters()["score"] == "B+Resign"));
        res.add("file", first.getFile());
    }
    // Game is finished, send the result
    res.type(Result::Win);

    first.gameQuit();
    second.gameQuit();
    return res;
}
