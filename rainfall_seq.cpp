#include <fstream>
#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include <limits.h>
#include <algorithm>
#include <time.h>
#include <sys/time.h>
#include <iomanip>

using namespace std;

double calc_time(struct timeval start, struct timeval end) {
  double start_sec = (double)start.tv_sec + (double)start.tv_usec / 1000000.0 ;
  double end_sec = (double)end.tv_sec + (double)end.tv_usec / 1000000.0;

  if (end_sec < start_sec) {
    return 0;
  } else {
    return end_sec - start_sec;
  }
}

void PrintMatrix(vector<vector<float> > & current) {
  for(auto & i : current) {
    for(auto j :i) {
      cout << j << ' ';
    }
    cout << endl;
  }
}
struct rain{
  float current;
  float absorp;
  float update;
  vector<pair<int, int> > direction;
  rain():current(0), absorp(0), update(0){}
};
int main(int argc, char ** argv) {
  if(argc != 6) {
    cerr << "Wrong Input" << endl;
    exit(-1);
  }
  int thread_num = stoi(argv[1]);
  int time_step = stoi(argv[2]);
  float absorp_rate = stof(argv[3]);
  int dimension = stoi(argv[4]);
  string elevation_file = argv[5];
  vector<vector<rain> > rain_map(dimension, vector<rain>(dimension));
  vector<vector<int> > lanscape(dimension, \
				vector<int>(dimension, 0));
  vector<pair<int, int> > dir;
  dir.push_back({-1, 0});// 1 up
  dir.push_back({1, 0});// 2 down
  dir.push_back({0, -1});// 3 left
  dir.push_back({0, 1});// 4 right
  
  // read the lanscape
  ifstream fin(elevation_file);
  for(int i = 0; i < dimension; ++i) {
    for(int j = 0; j < dimension; ++j) {
      fin >> lanscape[i][j];
    }
  }

  // set direction
  for(int i = 0; i < dimension; ++i) {
    for(int j = 0; j < dimension; ++j) {
     
      int min_height = lanscape[i][j];
      for(auto d: dir) {
	int x = i + d.first;
	int y = j + d.second;
	if(x >= 0 && x < dimension && \
	   y >= 0 && y < dimension )  {
	  min_height = min(min_height, lanscape[x][y]);
	}
      }
      if(min_height == lanscape[i][j]) continue;
      for(auto d: dir) {
        int x = i + d.first;
        int y = j + d.second;
        if(x >= 0 && x < dimension && \
	   y >= 0 && y < dimension && \
	   lanscape[x][y] == min_height) {
	  rain_map[i][j].direction.push_back({x, y});
	}
      }
     
    }
  }

  int total_slot = dimension * dimension;

  // rain simulation
  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);
  
  int flag = 1;
  if(time_step <= 0) flag = 0;
  int complete_time = 0;
  while(flag) {
    
    for(int i = 0; i < dimension; ++i) {
      for(int j = 0; j < dimension; ++j) {
	// rain drop
	if(time_step > 0) {
	  rain_map[i][j].current += 1;
	}
	// absorp
	if(rain_map[i][j].current > absorp_rate) {
	  rain_map[i][j].current -= absorp_rate;
	  rain_map[i][j].absorp += absorp_rate;
	} else  {
	  rain_map[i][j].absorp += rain_map[i][j].current;
	  rain_map[i][j].current = 0;
	}
	// trickle if there is a direction heading to
	float trickle = 0;
	if(rain_map[i][j].direction.size() > 0) {
	  if(rain_map[i][j].current > 1.0) {
	    rain_map[i][j].current -= 1.0;
	    trickle = 1.0;
	  } else {
	    trickle = rain_map[i][j].current;
	    rain_map[i][j].current = 0.0;
	  }
	  trickle /= rain_map[i][j].direction.size();
	  for(auto d: rain_map[i][j].direction) {
	    rain_map[d.first][d.second].update += trickle;
	  }
	}

      }
    }

    if(time_step > 0) time_step -= 1;
    complete_time += 1;
    int count = 0;
    // update the merge
    //PrintMatrix(update);
    //cout << endl;
    
    for(int i = 0; i < dimension; ++i) {
      for(int j = 0; j < dimension; ++j) {
	rain_map[i][j].current += rain_map[i][j].update;
	rain_map[i][j].update = 0;
	if(rain_map[i][j].current == 0.0) count += 1;
      }
    }
    
    if(time_step <= 0 && count == total_slot) flag = 0;
    //if(complete_time > 10) break;
  }
  gettimeofday(&end_time, NULL);
  double run_time = calc_time(start_time, end_time);

  // result
  cout << "Rainfall simulation took " << complete_time << \
    " time steps to complete." << endl;
  cout << "Runtime = " << run_time << " seconds" << endl;
  cout << "\n";
  cout << "The following grid shows the number of raindrops absorbed at each point:" << endl;
  for (unsigned i=0; i < dimension; i++) {
    for (unsigned j=0; j < dimension; j++) {
      cout << setw(8) << setprecision(6) << rain_map[i][j].absorp;
    } //for j                                                                                               
    cout << endl;
  } //for i     

}
