/*************************************************
Author: Tianwen Jiang
Date: 2016-01-26
Description: the Test for kb_embedding program
**************************************************/

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <algorithm>

using namespace std;

typedef pair<int, double> PAIR;  
  
bool cmp_by_value(const PAIR& lhs, const PAIR& rhs) {  
  return lhs.second < rhs.second;  
}  
  
struct CmpByValue {  
  bool operator()(const PAIR& lhs, const PAIR& rhs) {  
    return lhs.second < rhs.second;  
  }  
}; 

map<string, int> entity2id, relation2id;
map<int, string> id2entity, id2relation;
int entity_num, relation_num;

vector<vector<double> > entity_vec, relation_vec;
map<pair<int, int>, int> in_KB;

char buf[100000];
int L1_flag = 1;
int n = 100;
string version;

double sqr(double x)
{
    return x*x;
}

/*************************************************
Function: load_KB()
Description: load KB data from file
Input:
Output:
Return:
Others:
*************************************************/
void load_KB()
{
    FILE *KB_file = fopen("../data/train.txt","r");
    
    const char * split = "\t";
    char * p;
	while(!feof(KB_file))
    {
        fgets(buf, sizeof(buf), KB_file); 
        
        p = strtok(buf, split);
        string h = p;
        p = strtok(NULL, split);
        if (p==NULL)
            continue;
        string t = p;
        //cout << t << endl;
        p = strtok(NULL, split);
        string r = p;
        //r = r.replace('\n', '');
        //cout << r.length() <<endl;
        r = r.substr(0, r.length()-1);

        if (entity2id.count(h)==0)
            cout << "no entity: " << h << endl;
        if (entity2id.count(t)==0)
            cout << "no entity: " << t << endl;

        if (relation2id.count(r)==0)
            cout << "no relation: " << t << endl;

        in_KB[make_pair(entity2id[h], entity2id[t])] = 1;
    }
    fclose(KB_file);
}

/*************************************************
Function: load_entity_relation_data()
Description: load entity and relation data from file
Input:
Output: the number of entity and relation
Return:
Others:
*************************************************/
void load_entity_relation_data()
{
    FILE *entity_file = fopen("../data/entity2id.txt","r");
	FILE *relation_file = fopen("../data/relation2id.txt","r");

    int id;
    const char * split = "\t";
    char * p;
    
	while(!feof(entity_file))
    {
        fgets(buf, sizeof(buf), entity_file); 
        
        p = strtok(buf, split);
        string s = p;
        //cout << s << endl;
        p = strtok(NULL, split);
        if (p==NULL)
            continue;
        id = atoi(p);
        //cout << id << endl;

        entity2id[s] = id;
        id2entity[id] = s;
        entity_num++;
    }
    cout << "number of entity = " << entity_num << endl;
    fclose(entity_file);

	while(!feof(relation_file))
    {
        fgets(buf, sizeof(buf), relation_file); 
        
        p = strtok(buf, split);
        string s = p;
        //cout << s << endl;
        p = strtok(NULL, split);
        if (p==NULL)
            continue;
        id = atoi(p);
        //cout << id << endl;
        relation2id[s] = id;
        id2relation[id] = s;
        relation_num++;
    }
    cout << "number of relation = " << relation_num << endl;
    fclose(relation_file);
}

void load_entity_relation_vec()
{
    FILE *relation_vec_file = fopen(("./model/model_e2/relation2vec."+version).c_str(), "r");
    FILE *entity_vec_file = fopen(("./model/model_e2/entity2vec."+version).c_str(), "r");

    relation_vec.resize(relation_num);
    for (int i=0; i<relation_num;i++)
    {
        relation_vec[i].resize(n);
        for (int ii=0; ii<n; ii++){
            fscanf(relation_vec_file, "%lf", &relation_vec[i][ii]);
            //cout << relation_vec[i][ii] << endl;
        }
        if (i%10000==0)
            cout << "[info] relation_vec load " << i << " done" << endl;
    }
    entity_vec.resize(entity_num);
    for (int i=0; i<entity_num;i++)
    {
        entity_vec[i].resize(n);
        for (int ii=0; ii<n; ii++)
            fscanf(entity_vec_file, "%lf", &entity_vec[i][ii]);
        if ((i % 100000) == 0)
            cout << "[info] entity_vec load " << i << " done" << endl;
    }
    fclose(relation_vec_file);
    fclose(entity_vec_file);
}

double calc_distance(int h, int r, int t)
{
    double distance = 0;
    if (L1_flag == 1)
        for (int i = 0; i < n; i++)
            distance += fabs(entity_vec[h][i] + relation_vec[r][i] - entity_vec[t][i]);
    else
        for (int i = 0; i < n; i++){
            distance += sqr(entity_vec[h][i] + relation_vec[r][i] - entity_vec[t][i]);
        }
    return distance;
}

/*************************************************
Function: link_prediction()
Description: predict the ranked_link_list for given entitys
Input:
    entity1_id: int
    entity2_id: int
Output:
Return: the ranked_link_map
Others:
*************************************************/
map<int, double> link_prediction(int entity1_id, int entity2_id, double threshold, int num)
{
    map<int, double> ranked_link_map;
    double min = 10000000;
    int r_id = -1;
    double distance;
    
    for(int j = 0; j < num; j++)
    {
        for(int i = 0; i < relation_num; i++)
        {
            if (ranked_link_map.find(i) != ranked_link_map.end())
                continue;
            distance = calc_distance(entity1_id, i, entity2_id);
            if(distance < min)
            {
                min = distance;
                r_id = i;
            }
        }
        if (min >= threshold)
            break;
        ranked_link_map[r_id] = min;
        min = 10000000;        
    }
    
    return ranked_link_map;
}

/*************************************************
Function: entity_prediction()
Description: predict the ranked_link_list for given entitys
Input:
    entity1_id: int
    r_id: int
Output:
Return: the ranked_entity_map
Others:
*************************************************/
map<int, double> entity_prediction(int entity1_id, int r_id, int num)
{
    map<int, double> ranked_entity_map;
    double min = 10000000;
    int entity2_id = -1;
    double distance;
    
    for(int j = 0; j < num; j++)
    {
        for(int i = 0; i < entity_num; i++)
        {
            if (ranked_entity_map.find(i) != ranked_entity_map.end())
                continue;
            distance = calc_distance(entity1_id, r_id, i);
            if(distance < min)
            {
                min = distance;
                entity2_id = i;
            }
        }
        ranked_entity_map[entity2_id] = min;
        min = 10000000;        
    }
    
    return ranked_entity_map;
}

/*************************************************
Function: find_new_triple()
Description: due to the given KB and it's vector representation find new triple
Input:
    double threshold: the threshold of score for predictation
Output:
Return: 
Others:
*************************************************/
void find_new_triple()
{
    map<int, double> ranked_map;
    vector<PAIR> result_score_vec;
    ofstream new_KB_file;
    new_KB_file.open("KB_new.txt");
    
    for (int i = 0; i < entity_num; i++)
    {
        int count = 0;
        for (int j = 0; j < entity_num; j++)
        {
            if (in_KB.count(make_pair(i, j)) != 0 || i==j)
                continue;
            result_score_vec.clear();
            ranked_map = link_prediction(i, j, 0.5, 10);
            if (ranked_map.size()!=0)
            {
                for (map<int, double>::iterator it=ranked_map.begin(); it!=ranked_map.end(); ++it) {  
                    result_score_vec.push_back(make_pair(it->first, it->second));  
                }  
                sort(result_score_vec.begin(), result_score_vec.end(), CmpByValue());  
                new_KB_file << id2entity[i] << "==>" << id2entity[j];            
                for (vector<PAIR>::iterator it=result_score_vec.begin(); it!=result_score_vec.end(); ++it) {  
                    new_KB_file << "####" << id2relation[it->first] << ":" << it->second;  
                }
                new_KB_file << endl;
                new_KB_file.flush();
                count ++;
                if (count % 10 == 0)
                {
                    cout << id2entity[i] << ": " << "find " << count << endl;
                }
            }
        }
    }
}

/*************************************************
Function: have_arg()
Description: acquire the value for the given arg name
Input:
    char *str: the given arg name
    int argc: the number of the arguments
    char **argv: the arguments
Output:
Return:
    int index: the index of the value for the given arg name
Others:
*************************************************/
int have_arg(char *str, int argc, char**argv)
{
    for (int index = 0; index < argc; index++)
    {
        if (!strcmp(str, argv[index]))
        {
            if (index == argc - 1)
            {
                cout << "no such argument!" << endl;
                exit(1);
            }
            return index;
        }
    }
    return -1;
}

/*************************************************
Function: main()
Description: main control program
Input:
    int argc: the number of the arguments
    char **argv: the arguments
Output: the setting of model
Return:
    the state of excution
Others:
*************************************************/
int main(int argc, char **argv)
{
    int index;
    int method = 1;
    if ((index = have_arg((char *)"-size", argc, argv)) > 0) n = atoi(argv[index+1]);
    if ((index = have_arg((char *)"-method", argc, argv)) > 0) method = atoi(argv[index+1]);
    if ((index = have_arg((char *)"-L1", argc, argv)) > 0) L1_flag = atoi(argv[index+1]);

    cout << "size = " << n << endl;
    if (method == 1)
    {
        cout << "method = " << "bern" << endl;
        version = "bern";
    }
    else
    {
        cout << "method = " << "unif" << endl;
        version = "unif";
    }
    if (L1_flag == 1)
        cout << "use L1 to calculate distance." << endl;
    else
        cout << "use L2 to calculate distance." << endl;

    load_entity_relation_data();
    
    cout << "load entity and relation embedding data ... ..." << endl;
    load_entity_relation_vec();
    cout << "load ok." << endl;
    
    find_new_triple();

    return 0;
}
