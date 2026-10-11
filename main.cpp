#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <queue>

using namespace std;

enum class Status {
    Accepted,
    Wrong_Answer,
    Runtime_Error,
    Time_Limit_Exceed
};

Status stringToStatus(const string& status) {
    if (status == "Accepted") return Status::Accepted;
    if (status == "Wrong_Answer") return Status::Wrong_Answer;
    if (status == "Runtime_Error") return Status::Runtime_Error;
    if (status == "Time_Limit_Exceed") return Status::Time_Limit_Exceed;
    return Status::Wrong_Answer; // default
}

string statusToString(Status status) {
    switch (status) {
        case Status::Accepted: return "Accepted";
        case Status::Wrong_Answer: return "Wrong_Answer";
        case Status::Runtime_Error: return "Runtime_Error";
        case Status::Time_Limit_Exceed: return "Time_Limit_Exceed";
    }
    return "Wrong_Answer";
}

struct Submission {
    string problem;
    Status status;
    int time;
    
    Submission(string p, Status s, int t) : problem(p), status(s), time(t) {}
};

struct ProblemStatus {
    int wrong_attempts = 0;
    int solve_time = -1;
    bool solved = false;
    bool frozen = false;
    int frozen_submissions = 0;
    
    void addSubmission(Status status, int time, bool is_frozen_period) {
        if (solved) return;
        
        if (is_frozen_period) {
            frozen_submissions++;
            if (status == Status::Accepted) {
                // This is a hidden solve during freeze
                solved = true;
                solve_time = time;
            }
        } else {
            if (status == Status::Accepted) {
                solved = true;
                solve_time = time;
            } else {
                wrong_attempts++;
            }
        }
    }
    
    int getPenalty() const {
        if (!solved) return 0;
        return 20 * wrong_attempts + solve_time;
    }
    
    string getDisplayString() const {
        if (frozen) {
            if (wrong_attempts == 0) return "0/" + to_string(frozen_submissions);
            return to_string(wrong_attempts) + "/" + to_string(frozen_submissions);
        } else if (solved) {
            if (wrong_attempts == 0) return "+";
            return "+" + to_string(wrong_attempts);
        } else {
            if (wrong_attempts == 0) return ".";
            return "-" + to_string(wrong_attempts);
        }
    }
};

struct Team {
    string name;
    map<char, ProblemStatus> problems;
    vector<Submission> submissions;
    
    Team() = default;
    Team(string n) : name(n) {}
    
    void addSubmission(char problem, Status status, int time, bool is_frozen_period) {
        submissions.emplace_back(string(1, problem), status, time);
        problems[problem].addSubmission(status, time, is_frozen_period);
    }
    
    int getSolvedCount() const {
        int count = 0;
        for (const auto& p : problems) {
            if (p.second.solved && !p.second.frozen) {
                count++;
            }
        }
        return count;
    }
    
    int getTotalPenalty() const {
        int total = 0;
        for (const auto& p : problems) {
            if (p.second.solved && !p.second.frozen) {
                total += p.second.getPenalty();
            }
        }
        return total;
    }
    
    vector<int> getSolveTimes() const {
        vector<int> times;
        for (const auto& p : problems) {
            if (p.second.solved && !p.second.frozen) {
                times.push_back(p.second.solve_time);
            }
        }
        sort(times.rbegin(), times.rend());
        return times;
    }
    
    bool hasFrozenProblems() const {
        for (const auto& p : problems) {
            if (p.second.frozen) return true;
        }
        return false;
    }
    
    char getSmallestFrozenProblem() const {
        char smallest = 'Z' + 1;
        for (const auto& p : problems) {
            if (p.second.frozen && p.first < smallest) {
                smallest = p.first;
            }
        }
        return smallest;
    }
};

struct TeamRanking {
    string name;
    int solved;
    int penalty;
    vector<int> solve_times;
    
    bool operator<(const TeamRanking& other) const {
        if (solved != other.solved) return solved > other.solved;
        if (penalty != other.penalty) return penalty < other.penalty;
        
        // Compare solve times in descending order
        for (size_t i = 0; i < min(solve_times.size(), other.solve_times.size()); i++) {
            if (solve_times[i] != other.solve_times[i]) {
                return solve_times[i] < other.solve_times[i];
            }
        }
        if (solve_times.size() != other.solve_times.size()) {
            return solve_times.size() < other.solve_times.size();
        }
        
        return name < other.name;
    }
};

class ICPCManagementSystem {
private:
    map<string, Team> teams;
    bool competition_started = false;
    int duration_time = 0;
    int problem_count = 0;
    bool frozen = false;
    int freeze_time = -1;
    bool scoreboard_flushed = false;
    vector<TeamRanking> last_rankings;
    int current_time = 0;
    
public:
    void addTeam(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.count(team_name)) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        teams[team_name] = Team(team_name);
        cout << "[Info]Add successfully.\n";
    }
    
    void startCompetition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        competition_started = true;
        duration_time = duration;
        problem_count = problems;
        cout << "[Info]Competition starts.\n";
    }
    
    void submit(const string& problem, const string& team_name, const string& status, int time) {
        if (!teams.count(team_name)) return;
        
        current_time = time;
        char problem_char = problem[0];
        Status status_enum = stringToStatus(status);
        bool is_frozen_period = frozen && time >= freeze_time;
        
        // Mark problem as frozen if this is the first submission after freeze
        if (is_frozen_period) {
            auto& prob_status = teams[team_name].problems[problem_char];
            if (!prob_status.frozen && !prob_status.solved) {
                prob_status.frozen = true;
            }
        }
        
        teams[team_name].addSubmission(problem_char, status_enum, time, is_frozen_period);
    }
    
    void flushScoreboard() {
        cout << "[Info]Flush scoreboard.\n";
        updateRankings();
        scoreboard_flushed = true;
    }
    
    void freezeScoreboard() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        frozen = true;
        freeze_time = current_time;
        
        // Don't mark problems as frozen yet - they become frozen when submitted after freeze
        cout << "[Info]Freeze scoreboard.\n";
    }
    
    void scrollScoreboard() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }
        
        cout << "[Info]Scroll scoreboard.\n";
        
        // First flush
        updateRankings();
        
        // Output scoreboard before scrolling
        outputScoreboard();
        
        // Track ranking changes
        vector<string> changes;
        
        // Perform scrolling
        while (true) {
            // Find lowest-ranked team with frozen problems
            int lowest_frozen_idx = -1;
            for (int i = static_cast<int>(last_rankings.size()) - 1; i >= 0; i--) {
                if (teams[last_rankings[i].name].hasFrozenProblems()) {
                    lowest_frozen_idx = i;
                    break;
                }
            }
            
            if (lowest_frozen_idx == -1) break;
            
            string team_name = last_rankings[lowest_frozen_idx].name;
            char problem = teams[team_name].getSmallestFrozenProblem();
            
            // Unfreeze this problem
            teams[team_name].problems[problem].frozen = false;
            
            // Update rankings
            vector<TeamRanking> old_rankings = last_rankings;
            updateRankings();
            
            // Check if ranking changed
            int new_rank = -1;
            for (size_t i = 0; i < last_rankings.size(); i++) {
                if (last_rankings[i].name == team_name) {
                    new_rank = static_cast<int>(i);
                    break;
                }
            }
            
            if (new_rank != lowest_frozen_idx && new_rank != -1) {
                string replaced_team = old_rankings[new_rank].name;
                changes.push_back(team_name + " " + replaced_team + " " + 
                                to_string(last_rankings[new_rank].solved) + " " + 
                                to_string(last_rankings[new_rank].penalty));
            }
        }
        
        // Output ranking changes
        for (const string& change : changes) {
            cout << change << "\n";
        }
        
        // Output final scoreboard
        outputScoreboard();
        
        frozen = false;
    }
    
    void queryRanking(const string& team_name) {
        if (!teams.count(team_name)) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        
        if (!scoreboard_flushed) {
            // Before first flush, ranking is based on lexicographic order
            vector<string> team_names;
            for (const auto& t : teams) {
                team_names.push_back(t.first);
            }
            sort(team_names.begin(), team_names.end());
            
            for (size_t i = 0; i < team_names.size(); i++) {
                if (team_names[i] == team_name) {
                    cout << "[" << team_name << "] NOW AT RANKING [" << static_cast<int>(i + 1) << "]\n";
                    return;
                }
            }
        } else {
            // Use last rankings
            for (size_t i = 0; i < last_rankings.size(); i++) {
                if (last_rankings[i].name == team_name) {
                    cout << "[" << team_name << "] NOW AT RANKING [" << static_cast<int>(i + 1) << "]\n";
                    return;
                }
            }
        }
    }
    
    void querySubmission(const string& team_name, const string& problem, const string& status) {
        if (!teams.count(team_name)) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query submission.\n";
        
        const auto& submissions = teams[team_name].submissions;
        const Submission* result = nullptr;
        
        for (const auto& sub : submissions) {
            bool problem_match = (problem == "ALL" || sub.problem == problem);
            bool status_match = (status == "ALL" || statusToString(sub.status) == status);
            
            if (problem_match && status_match) {
                result = &sub;
            }
        }
        
        if (result) {
            cout << "[" << team_name << "] [" << result->problem << "] [" 
                 << statusToString(result->status) << "] [" << result->time << "]\n";
        } else {
            cout << "Cannot find any submission.\n";
        }
    }
    
    void endCompetition() {
        cout << "[Info]Competition ends.\n";
    }
    
private:
    void updateRankings() {
        last_rankings.clear();
        for (const auto& t : teams) {
            TeamRanking tr;
            tr.name = t.first;
            tr.solved = t.second.getSolvedCount();
            tr.penalty = t.second.getTotalPenalty();
            tr.solve_times = t.second.getSolveTimes();
            last_rankings.push_back(tr);
        }
        sort(last_rankings.begin(), last_rankings.end());
    }
    
    void outputScoreboard() {
        for (size_t i = 0; i < last_rankings.size(); i++) {
            const auto& tr = last_rankings[i];
            const auto& team = teams[tr.name];
            
            cout << tr.name << " " << static_cast<int>(i + 1) << " " << tr.solved << " " << tr.penalty;
            
            for (char c = 'A'; c < 'A' + problem_count; c++) {
                cout << " ";
                if (team.problems.count(c)) {
                    cout << team.problems.at(c).getDisplayString();
                } else {
                    cout << ".";
                }
            }
            cout << "\n";
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    ICPCManagementSystem system;
    string line;
    
    while (getline(cin, line)) {
        if (line.empty()) continue;
        
        istringstream iss(line);
        string command;
        iss >> command;
        
        if (command == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            system.addTeam(team_name);
        } else if (command == "START") {
            string duration_str, problem_str;
            int duration, problems;
            iss >> duration_str >> duration >> problem_str >> problems;
            system.startCompetition(duration, problems);
        } else if (command == "SUBMIT") {
            string problem, by, team_name, with, status_str, at;
            int time;
            iss >> problem >> by >> team_name >> with >> status_str >> at >> time;
            system.submit(problem, team_name, status_str, time);
        } else if (command == "FLUSH") {
            system.flushScoreboard();
        } else if (command == "FREEZE") {
            system.freezeScoreboard();
        } else if (command == "SCROLL") {
            system.scrollScoreboard();
        } else if (command == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            system.queryRanking(team_name);
        } else if (command == "QUERY_SUBMISSION") {
            string team_name, where, problem_str, and_str, status_str;
            iss >> team_name >> where >> problem_str >> and_str >> status_str;
            
            // Parse problem and status
            string problem = problem_str.substr(8); // Remove "PROBLEM="
            string status = status_str.substr(7);   // Remove "STATUS="
            
            system.querySubmission(team_name, problem, status);
        } else if (command == "END") {
            system.endCompetition();
            break;
        }
    }
    
    return 0;
}