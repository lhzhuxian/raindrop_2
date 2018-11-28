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
#include <pthread.h>
using namespace std;

#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


// global
int thread_num;
int complete_step;
int dimension;
float absorp_rate;
pthread_barrier_t barrier;
int * flag;
int total_slot;

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
  for( auto & i : current) {
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

struct p_rain {
  vector<vector<rain> > & rain_map;
  pthread_mutex_t * upper_lock;
  pthread_mutex_t * lower_lock;
  int start;
  int end;
  int time_step;
  int id;
  int complete_time;
  p_rain(vector<vector<rain> > & _rain_map, \
	 pthread_mutex_t * _upper_lock,	       \
	 pthread_mutex_t * _lower_lock,	       \
	 int _start, int _end, int _time_step,  \
	 int _id): rain_map(_rain_map), upper_lock(_upper_lock),\
		   lower_lock(_lower_lock), start(_start), end(_end),\
		   time_step(_time_step), id(_id), complete_time(0){}
};

bool CheckFlag() {
  for(int i = 0; i < thread_num; ++i) {
    if(flag[i] == 1) return true;
  }
  return false;
}

void SimHelper (p_rain * arg, int i , int j) {
  // rain drop
  if(arg->time_step > 0) {
    arg->rain_map[i][j].current += 1;
  }
  // absorp
  if(arg->rain_map[i][j].current > absorp_rate) {
    arg->rain_map[i][j].current -= absorp_rate;
          arg->rain_map[i][j].absorp += absorp_rate;
  } else  {
    arg->rain_map[i][j].absorp += arg->rain_map[i][j].current;
    arg->rain_map[i][j].current = 0;
  }
  // trickle if there is a direction heading to
  float trickle = 0;
  if(arg->rain_map[i][j].direction.size() > 0) {
    if(arg->rain_map[i][j].current > 1.0) {
      arg->rain_map[i][j].current -= 1.0;
      trickle = 1.0;
    } else {
      trickle = arg->rain_map[i][j].current;
            arg->rain_map[i][j].current = 0.0;
    }
    trickle /= arg->rain_map[i][j].direction.size();
    for(auto d: arg->rain_map[i][j].direction) {
      arg->rain_map[d.first][d.second].update += trickle;
    }
     
  }
}

void * Simulation(void * _arg) {
  
  p_rain * arg = (p_rain *) _arg;
  int total_slot = (arg->end - arg->start + 1) * dimension;
  while(CheckFlag()) {
    pthread_barrier_wait(&barrier);
    if(arg->upper_lock) pthread_mutex_lock(arg->upper_lock);
    for(int i = arg->start; i <= arg->start + 1; ++i) {
      for(int j = 0; j < dimension; ++j) {
	SimHelper(arg, i, j);
      }
    }
    if(arg->upper_lock) pthread_mutex_unlock(arg->upper_lock);
    
    for(int i = arg->start + 2; i <= arg->end - 2; ++i) {
      for(int j = 0; j < dimension; ++j) {
	SimHelper(arg, i, j);
      }
    }

    if(arg->lower_lock) pthread_mutex_lock(arg->lower_lock);
    for(int i = arg->end - 1; i <= arg->end; ++i) {
      for(int j = 0; j < dimension; ++j) {
        SimHelper(arg, i, j);
      }
    }
    if(arg->lower_lock) pthread_mutex_unlock(arg->lower_lock);
    
    if(arg->time_step > 0) arg->time_step -= 1;
    arg->complete_time += 1;
    int count = 0;
    // update the merge
	//PrintMatrix(update);
	//cout << endl;
    pthread_barrier_wait(&barrier);
    for(int i = arg->start; i <= arg->end; ++i) {
      for(int j = 0; j < dimension; ++j) {
	arg->rain_map[i][j].current += arg->rain_map[i][j].update;
	arg->rain_map[i][j].update = 0;
	if(arg->rain_map[i][j].current == 0.0) count += 1;
      }
    }
    
    if(arg->time_step <= 0 && count == total_slot) flag[arg->id] = 0;
    pthread_barrier_wait(&barrier);
  }
  
}

int main(int argc, char ** argv) {
  if(argc != 6) {
    cerr << "Wrong Input" << endl;
    exit(-1);
  }
  thread_num = stoi(argv[1]);
  if(thread_num < 2) {
    cerr << "Thread num must larger than or equal to 2" << endl;
    exit(-1);
  }
  int time_step = stoi(argv[2]);
  absorp_rate = stof(argv[3]);
  dimension = stoi(argv[4]);
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
  // set up thread
  flag = new int[thread_num];
  pthread_mutex_t * lock = new pthread_mutex_t[thread_num];
  p_rain ** arg = new p_rain * [thread_num];
  int size = dimension / thread_num;
  int start = 0;
  int end = size;
  for(int i = 0; i < thread_num; ++i) {
    flag[i] = 1;
    lock[i] = PTHREAD_MUTEX_INITIALIZER;
    if(i == 0) {
      arg[i] = new p_rain(rain_map, NULL, &lock[i], start, end, time_step, i);
    } else if (i == thread_num - 1) {
      arg[i] = new p_rain(rain_map, &lock[i], NULL, start, dimension - 1, time_step, i);
    } else {
      arg[i] = new p_rain(rain_map, &lock[i], &lock[i + 1], start, end, time_step, i);
    }
    start = end + 1;
    end = start + size;
  }
  pthread_barrier_init(&barrier, NULL, thread_num);
  pthread_t * thread = new pthread_t[thread_num];
  
  // rain simulation
  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);
  for(int i = 0; i < thread_num; ++i) {
    int s = pthread_create(thread + i, NULL, &Simulation, (void*)(arg[i]));
    if (s != 0)
               handle_error_en(s, "pthread_create");
  }
  for(int i = 0; i < thread_num; ++i) {
    pthread_join(thread[i], NULL);
  }
  gettimeofday(&end_time, NULL);
  double run_time = calc_time(start_time, end_time);

  // result
  cout << "Rainfall simulation took " << arg[0]->complete_time << \
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
