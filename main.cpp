//
//  main.cpp
//  rappor_for_location
//
//  Created by Kyunam Cho on 11/29/23.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <cmath>
#include <format>
#include <functional>
#include <random>
#include <ctime>
#include <iomanip>

using namespace std;

typedef struct global_setting{
    float f = 0.5f;
    float p = 0.5f;
    float q = 0.75f;
    int h = 6;
    int k = 256;
    int m = 1;
    int precision_for_round = 3;
} GLOBAL_SETTING;

GLOBAL_SETTING g_setting;

const int max_filter_size = 8192;
int shift_value = 254;
char bloom_filer[max_filter_size] = { '0', };
vector<function<int(int, int)>> hash_functions;
const string local_path = "/Users/mystous/projects/rappor_for_location/rappor_for_location/";
const int k_size = max_filter_size;
const string stastics_head = ",accuracy,find_count,total_count,hash_functions,bloomfilter_size,f,p,q,precision,e_p,e_1";

// Application configration
const int line_skip_unit = 30000;
const bool show_progress = false;

// Experiment parameters
const int repeat_count = 1;
const int h_value_count = 6;
int h_value[h_value_count] = { 1, 2, 3, 4, 5, 6 };
const int k_value_count = 4;
int k_value[k_value_count] = { 32, 64, 128, 256 };
const int pr_value_count = 2;
int pr_value[pr_value_count] = { 3, 4 };
const int f_value_count = 10;
float f_value[f_value_count] = { 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 };
const int p_value_count = 9;
float p_value[p_value_count] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 };
const int q_value_count = 18;
float q_value[q_value_count] = { 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95 };

bool random_decision(double probability) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::bernoulli_distribution dist(probability);
    return dist(gen);
}

float calcu_epsilon_permanent(GLOBAL_SETTING setting){
    float epsilon_permanent = 0.0f;
    
    epsilon_permanent = 2 * setting.h * log(( 1 - 0.5 * setting.f ) / ( 0.5 * setting.f ));
    return epsilon_permanent;
}

float calcu_epsilon_instantaneous(GLOBAL_SETTING setting){
    float epsilon_instantaneous = 0.0f;
    float p_star, q_star;

    p_star = 0.5 * setting.f * ( setting.p + setting.q ) + ( 1 - setting.f ) * setting.p;
    q_star = 0.5 * setting.f * ( setting.p + setting.q ) + ( 1 - setting.f ) * setting.q;
    epsilon_instantaneous = setting.h * log10(( q_star * ( 1 - p_star )) / ( p_star * ( 1 - q_star )));
    return epsilon_instantaneous;
}

int quantize_coordinate(double coordinate, GLOBAL_SETTING setting) {
    double scale = pow(10.0, setting.precision_for_round);
    int new_coordination = round(coordinate * scale);
    if( new_coordination < 0 )
        new_coordination = 180 * pow(10, setting.precision_for_round) + (-1) * new_coordination;
    return new_coordination;
}

float convert_rounded_coordinate(double coordinate, GLOBAL_SETTING setting) {
    double scale = pow(10.0, setting.precision_for_round);
    
    int new_coordination = round(coordinate * scale);
    float converted_coordination = 0.;

    converted_coordination = new_coordination / pow(10, setting.precision_for_round);
    return converted_coordination;
}

void make_bloomfiler(int seed_value, GLOBAL_SETTING setting, bool initialize){
    if( initialize){
        for( int i = 0 ; i < setting.k ; ++i ){
            bloom_filer[i] = '0';
        }
    }
    
    for( int i = 0 ; i < setting.h ; ++i ){
        bloom_filer[hash_functions[i](seed_value, setting.k)] = '1';
    }
    bloom_filer[setting.k] = '\0';
}

void randomize_permanent(GLOBAL_SETTING setting){
    for( int i = 0 ; i < setting.k ; ++i ){
        if( !random_decision(1-setting.f) )
        {
            if( random_decision(setting.f/2))
                
                bloom_filer[i] = '1';
            else
                bloom_filer[i] = '0';
        }
    }
}

void randomize_instantaneous(GLOBAL_SETTING setting){
    for( int i = 0 ; i < setting.k ; ++i ){
        if( '1' == bloom_filer[i]  ){
            if( random_decision(setting.q))
                bloom_filer[i] = '1';
        }
        else{
            if( random_decision(setting.p))
                bloom_filer[i] = '1';
        }
    }
}

string make_bloomfiler_string(double coordination, GLOBAL_SETTING setting, bool initialize){
    int new_coordination = quantize_coordinate(coordination, setting);
    make_bloomfiler(new_coordination, setting, initialize);
    return bloom_filer;
}

string make_randomized_respose(double coordination, GLOBAL_SETTING setting, bool initialize){
    int new_coordination = quantize_coordinate(coordination, setting);
    make_bloomfiler(new_coordination, setting, initialize);
    randomize_permanent(setting);
    randomize_instantaneous(setting);
    return bloom_filer;
}

struct shift {
    int operator()(int operand, int mod_value) { 
        long long int mid_result = operand * shift_value;
        return mid_result % mod_value;
    }
};

struct shift_reverse {
    int operator()(int operand, int mod_value) {
        long long int mid_result = operand * 2;
        return mid_result % mod_value;
    }
};

struct simple_mod {
    int operator()(int operand, int mod_value) { return operand % mod_value; }
};

struct after_add_mod {
    int operator()(int operand, int mod_value) {
        long long int mid_result = operand  + shift_value;
        return mid_result % mod_value;
    }
};

struct after_mul_mod {
    int operator()(int operand, int mod_value) {
        long long int mid_result = operand  + shift_value + shift_value;
        return mid_result % mod_value;
    }
};

struct after_squre_mod {
    int operator()(int operand, int mod_value) {
        long long int mid_result = (operand % shift_value)  * (operand % shift_value);
        return mid_result % mod_value;
    }
};

void initialize_function_object(){
    
    hash_functions.push_back(shift());
    hash_functions.push_back(simple_mod());
    hash_functions.push_back(after_add_mod());
    hash_functions.push_back(shift_reverse());
    hash_functions.push_back(after_mul_mod());
    hash_functions.push_back(after_squre_mod());
}

void build_string_and_path(string &readfile, string &writefile, string &message, GLOBAL_SETTING setting, int try_number){
    readfile = local_path + "data";
    
    message =
    "f(" + to_string(setting.f) + ")_" +
    "p(" + to_string(setting.p) + ")_" +
    "q(" + to_string(setting.q) + ")_" +
    "h(" + to_string(setting.h) + ")_" +
    "k(" + to_string(setting.k) + ")_" +
    "m(" + to_string(setting.m) + ")_" +
    "precision_for_round(" + to_string(setting.precision_for_round) + ")_" +
    "experiment(" + to_string(try_number) + ")_";
    
    writefile = local_path + "e_p_" + to_string(calcu_epsilon_permanent(setting)) + "_e_1_" + to_string(calcu_epsilon_instantaneous(setting)) + message + "output.csv";
}

void read_data_and_change_coordinate(ifstream &read_file, ofstream &write_file, GLOBAL_SETTING setting, bool write_operation, function<string(double, GLOBAL_SETTING, bool)> operation_function){
    int csv_index = 0;
    bool title = true;
    std::string line;
    
    if (!read_file.is_open()) {
        cerr << "Unable to open file" << std::endl;
        return;
    }
    
    if (!write_file.is_open()) {
        std::cerr << "Unable to open target file" << std::endl;
        return;
    }
    int line_count = 0;
    
    while (std::getline(read_file, line)) {
        if ( line_count % line_skip_unit == 0 && true == show_progress)
            cout << "# " << line_count << " lines reading and processing" << endl;
        line_count++;

        std::istringstream s(line);
        if( title ){
            write_file << line << "\n";
            title = false;
            continue;
        }
        std::string field;
        std::vector<std::string> fields;
        
        while (getline(s, field, ',')) {
            fields.push_back(field);
        }
        
        csv_index = 0;
        string row = "";
        for (const auto& f : fields) {
            string element = f;
            bool initialize = false;
            if( 4 == csv_index )
                initialize = true;
            if( 4 == csv_index || 5 == csv_index ){
                row += element;
                row += ",";
                double coordination = stod(element);
                element = operation_function(coordination, g_setting, initialize);
            }
            
            csv_index++;
            if( initialize )
                continue;
            
            row += element;
            if( 8 != csv_index )
                row += ",";
        }
        
        write_file << row;
        write_file << "\n";
    }
}

void build_string_and_path_answer(string &encoding_bloom_filer, GLOBAL_SETTING setting){
    encoding_bloom_filer = local_path + "bloom_encoding_" +
    "h(" + to_string(setting.h) + ")_" +
    "k(" + to_string(setting.k) + ")_" +
    "precision_for_round(" + to_string(setting.precision_for_round) + ").csv";
}

void build_string_and_path_compare_result(string &result_file, GLOBAL_SETTING setting){
    result_file = local_path + "bloom_encoding_" +
    "h(" + to_string(setting.h) + ")_" +
    "k(" + to_string(setting.k) + ")_" +
    "precision_for_round(" + to_string(setting.precision_for_round) + ")_result.csv";
}

void build_answer_set(GLOBAL_SETTING setting){
    string original_data_file = local_path + "data";
    string encoding_bloom_filer;
    build_string_and_path_answer(encoding_bloom_filer, setting);
    
    ifstream read_file(original_data_file);
    ofstream write_file(encoding_bloom_filer);
    
    cout << encoding_bloom_filer << " operation started..." << endl;
    read_data_and_change_coordinate(read_file, write_file, g_setting, true, make_bloomfiler_string);
    
    read_file.close();
    write_file.close();
    cout << encoding_bloom_filer << " operation finished..." << endl;
}

void do_rappor(int try_number)
{
    string read_file_name, write_file_name, message;
    build_string_and_path(read_file_name, write_file_name, message, g_setting, try_number);
    ifstream read_file(read_file_name);
    ofstream write_file(write_file_name);
    
    cout << message << " operation started..." << endl;
    read_data_and_change_coordinate(read_file, write_file, g_setting, true, make_randomized_respose);
    
    read_file.close();
    write_file.close();
    cout << message << " operation finished..." << endl;
}

void build_bloom_filter_bool(string bloom_filter_string, vector<vector<bool>> &coordination){
    vector<bool> bloom_filer_bool;
    for( int i = 0 ; i < k_size ; ++i ){
        bloom_filer_bool.push_back(bloom_filter_string[i] == '1' ? true : false);
    }
    coordination.push_back(bloom_filer_bool);
}

void read_bloomfilter(string bloom_result, vector<vector<bool>> &bloom_convert){
    ifstream bloom_file(bloom_result);
    
    int csv_index = 0;
    bool title = true;
    std::string line;
    
    if (!bloom_file.is_open()) {
        cerr << "Unable to open file" << std::endl;
        return;
    }
    
    int line_count = 0;
    while (std::getline(bloom_file, line)) {
        if ( line_count % line_skip_unit == 0 && true == show_progress)
            cout << "# " << line_count << " lines reading and processing" << endl;
        line_count++;
        std::istringstream s(line);
        if( title ){
            title = false;
            continue;
        }
        std::string field;
        std::vector<std::string> fields;
        
        while (getline(s, field, ',')) {
            fields.push_back(field);
        }
        
        csv_index = 0;
        for (const auto& f : fields) {
            string element = f;
            if( 6 == csv_index )
                build_bloom_filter_bool(element, bloom_convert);
            
            csv_index++;
        }
    }
    
    bloom_file.close();
}

bool rappor_include_bloom(const auto &rappor_bloom, const auto &answer){
    bool finded = true;
    
    int index = 0;
    bool origin = false;
    for ( const auto &target : rappor_bloom ){
        origin = answer[index++];
        if ( false == origin )
            continue;
        if ( target != origin ){
            finded = false;
            break;
        }
    }
    return finded;
}

int find_bloom_filer_in_list(vector<vector<bool>> &rappor,vector<vector<bool>> &answer, vector<bool> &finded_index, GLOBAL_SETTING setting){
    int finding_count = 0;
    
    for( int i = 0 ; i < answer.size() ; ++i ){
        if ( i % line_skip_unit == 0 && true == show_progress)
            cout << "Checking: answer(" << i  << ")" << endl;
        vector<bool> rappor_bloom = rappor[i];
        vector<bool> bloom = answer[i];
        finded_index.push_back(rappor_include_bloom(rappor_bloom, bloom));
        if(finded_index[i])
            finding_count++;
    }
    
    return finding_count;
}

void compare_bloom_filer(vector<vector<bool>> &rappor,vector<vector<bool>> &answer, GLOBAL_SETTING setting, int count, ofstream &write_file, vector<bool> &finded){
    string new_header = ",ID,Severity,Start_Time,Start_Lat,Start_Lng,City,County,E_p,E_1";
    int finding_count = find_bloom_filer_in_list(rappor, answer, finded, setting);
    float percent = (float)finding_count/(float)answer.size();
    cout << endl << "Rappor show " << percent << " % results" << endl << endl;
    
    string stastics_body = to_string(count) + "," + to_string(percent) + "," +
    to_string(finding_count)  + "," + to_string(answer.size())  + "," +
    to_string(setting.h)  + "," + to_string(setting.k) + "," + to_string(setting.f) + "," +
    to_string(setting.p) + "," + to_string(setting.q) + "," + to_string(setting.precision_for_round) +
    "," + to_string(calcu_epsilon_permanent(setting)) + "," + to_string(calcu_epsilon_instantaneous(setting));

    write_file << stastics_body << "\n";
}

string ftos(float f, int nd) {
   ostringstream ostr;
    if( f < 0 )
        nd += 1;
   int tens = stoi("1" + string(nd, '0'));
   ostr << round(f*tens)/tens;
   return ostr.str();
}

void print_out_compare_result_to_answer(vector<bool> &finded, GLOBAL_SETTING setting){
    string answer_file_name, result_file_name;
    
    build_string_and_path_answer(answer_file_name, setting);
    build_string_and_path_compare_result(result_file_name, setting);
    
    ifstream answer_file(answer_file_name);
    ofstream result_file(result_file_name);
    
    int csv_index = 0;
    bool title = true;
    std::string line;
    
    if (!answer_file.is_open()) {
        cerr << "Unable to open file" << std::endl;
        return;
    }
    
    if (!result_file.is_open()) {
        std::cerr << "Unable to open target file" << std::endl;
        return;
    }
    int line_count = 0;
    
    while (std::getline(answer_file, line)) {
        line_count++;

        std::istringstream s(line);
        if( title ){
            result_file << ",ID,Severity,Start_Time,Start_Lat,rounded_latitude,Start_Lng,rounded_longitude,Bloomfilter,City,County,finded_in_RAPPOR,E_p,E_1" << "\n";
            title = false;
            continue;
        }
        std::string field;
        std::vector<std::string> fields;
        
        while (getline(s, field, ',')) {
            fields.push_back(field);
        }
        
        csv_index = 0;
        string row = "";
        for (const auto& f : fields) {
            string element = f;
            if( 4 == csv_index || 5 == csv_index ){
                row += element;
                row += ",";
                double coordination = stod(element);
                float converted_coordination = convert_rounded_coordinate(coordination, setting);
                //                element = ftos(converted_coordination, setting.precision_for_round);
                element = to_string(converted_coordination);
            }
            csv_index++;
            row += element;
            row += ",";
        }
        row += finded[line_count-1] ? "true" : "false";
        row += ",";
        row += to_string(calcu_epsilon_permanent(setting));
        row += ",";
        row += to_string(calcu_epsilon_instantaneous(setting));
    
        result_file << row;
        result_file << "\n";
    }
    
    answer_file.close();
    result_file.close();
    
    cout << "Write result to answer file is completed" << endl;
}

void check_rappor_result(string rappor_result, string answer_file, GLOBAL_SETTING setting, int count, ofstream &write_file, vector<bool> &finded){
    vector<vector<bool>> rappor, answer;
    
    cout << "Build bloom filer for " << rappor_result << endl;
    read_bloomfilter(rappor_result, rappor);
    cout << "Build bloom filer for " << answer_file << endl;
    read_bloomfilter(answer_file, answer);
    compare_bloom_filer(rappor, answer, setting, count, write_file, finded);
    print_out_compare_result_to_answer(finded, setting);
}

void check_rappor_result_total(int try_count, int count, ofstream &write_file, GLOBAL_SETTING setting){
    string rappor_result, answer_file, read_file_name, message;
    string result_file;
    vector<bool> finded;

    build_string_and_path_answer(answer_file, setting);
    build_string_and_path(read_file_name, rappor_result, message, setting, try_count);
    check_rappor_result(rappor_result, answer_file, setting, count, write_file, finded);
    build_string_and_path_compare_result(result_file, setting);

}

void build_string_and_path_with_time(string &message){
    time_t current_time = time(nullptr);
    string current_time_str = asctime(localtime(&current_time));

        if (!current_time_str.empty() && current_time_str.back() == '\n') {
            current_time_str.pop_back();
        }
    replace(current_time_str.begin(), current_time_str.end(), ':', '_');
    replace(current_time_str.begin(), current_time_str.end(), ' ', '_');
    message = local_path + "stastics_" + current_time_str + ".csv";
}

int main(int argc, const char * argv[]) {
    initialize_function_object();
    int count = 0, total = 0;
    string stastics_file_name;
    build_string_and_path_with_time(stastics_file_name);
    ofstream write_file(stastics_file_name);
    total = h_value_count * k_value_count * f_value_count * pr_value_count * p_value_count * q_value_count * repeat_count;
    
    if (!write_file.is_open()) {
        cout << "Static file can't create!" << endl;
        return 9;
    }
    
    write_file << stastics_head << "\n";
    
    for( int i = 0 ; i < h_value_count ; ++i){
        g_setting.h = h_value[i];
        for( int j = 0 ; j < k_value_count ; ++j){
            g_setting.k = k_value[j];
            for( int k = 0 ; k < f_value_count ; ++k ){
                g_setting.f = f_value[k];
                for( int pp = 0 ; pp < pr_value_count ; ++pp){
                    g_setting.precision_for_round = pr_value[pp];
                    for( int pv = 0 ; pv < p_value_count ; ++pv){
                        g_setting.p = p_value[pv];
                        for( int qv = 0 ; qv < q_value_count ; ++qv){
                            g_setting.q = q_value[qv];
                            for( int rp = 0 ; rp < repeat_count ; ++rp){
                                float percent = (float)++count / (float) total * 100;
                                cout << count << " / " << total << " (" << percent << "%)" << endl;
                                
                                float ep = calcu_epsilon_permanent(g_setting);
                                float e1 = calcu_epsilon_instantaneous(g_setting);
                                
                                if( ep < 0 || e1 < 0 ){
                                    cout << "Epsion values have to greater than 0 ep(" << ep << "), e1(" << e1 << ")" << endl << "This configration is skiped" << endl;
                                    continue;
                                }
                                
                                do_rappor(rp);
                                build_answer_set(g_setting);
                                check_rappor_result_total(rp, count, write_file, g_setting);
                            }
                        }
                    }
                }
            }
        }
    }
    write_file.close();
    cout << stastics_file_name << " includes data" << endl << "finished!" << endl;
    return 0;
}
