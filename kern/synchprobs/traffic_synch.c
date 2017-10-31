#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>

/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */
void update_intersection(int o, int d, bool b);
void wake_cars(int o);
int next_dir(int o);
void init_para(void);
void intersection_sync_init(void);
void intersection_sync_cleanup(void);
void intersection_before_entry(Direction origin, Direction destination);
void intersection_after_exit(Direction origin, Direction destination);
/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */
static volatile int intersection[4][4];
static volatile int wait_cars[4];
static struct lock *traffic_lock;
static struct cv *traffic_cv[4][4];


void update_intersection(int o, int d, bool b) {
  for (int i = 0; i < 4; i++) {
    if (i != o) {
      for (int j = 0; j < 4; j++) {
        if (!(i == d && j == o) && !(j != d && (j - i == -1 || j - i == 3 || d - o == -1 || d - o == 3))) {
	  	if(b) {
		intersection[i][j] += 1;
		} else {
		intersection[i][j] -= 1;
		}
        }
      }
    }
  }
}

int next_dir(int o) {
	if(o == 3) {
        o = 0;
	}else{
        o += 1;
        }  
	return o;
}

void wake_cars(int o) {
	o = next_dir(o);
	int tem = o;
	
	for (int i = 0; i < 4; i++) {
                if(traffic_cv[o][i] != NULL && intersection[o][i] == 0) {
                        cv_signal(traffic_cv[o][i], traffic_lock);
                }
        }

	o = next_dir(o);
	while (o != tem) {
	

	for (int i = 0; i < 4; i++) {
		if(traffic_cv[o][i] != NULL && intersection[o][i] == 0) {
			cv_signal(traffic_cv[o][i], traffic_lock);
		}
	}

	o = next_dir(o);
	}
}

void init_para(void) {
	for (int i = 0; i < 4; i++) {
		wait_cars[i] = 0;
		for (int j = 0; j < 4; j++) {
			if(i == j) {
				intersection[i][j] = -1;
				traffic_cv[i][j] = NULL;
			} else {
				intersection[i][j] = 0;
				int index = i * 4 + j;
				char cv_name[3];
				snprintf(cv_name, sizeof(cv_name), "%d", index);
				traffic_cv[i][j] = cv_create(cv_name);
				if(traffic_cv[i][j] == NULL) {
					panic("could not create traffic-cv: %d", index);
				}
			}
		}
	}
}


/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */

	traffic_lock = lock_create("traffic_lock");
	if(traffic_lock == NULL) {
		panic("could not create traffic lock");
	}
	
	init_para();	


  return;
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
	KASSERT(traffic_lock != NULL); 
	lock_destroy(traffic_lock);

	for (int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			if(traffic_cv[i][j] != NULL) {
				cv_destroy(traffic_cv[i][j]);
			}
		}
	}

 /* replace this default implementation with your own implementation */
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

void
intersection_before_entry(Direction origin, Direction destination) 
{
	KASSERT(traffic_lock != NULL);
	KASSERT(traffic_cv[origin][destination] != NULL);

	lock_acquire(traffic_lock);
	int a = 0;
	int tem = wait_cars[origin] + 1;
	while(intersection[origin][destination] > 0) {
		wait_cars[origin] = tem;
		a = 1;
		cv_wait(traffic_cv[origin][destination], traffic_lock);
	}
/*
	for (unsigned int i = 0; i < 4; i++) {
		if (i != origin) {
			for (unsigned int j = 0; j < 4; j++) {
				if (!(i == destination && j == origin) && !(j != destination && (origin == (destination + 1) % 4 || i == (j + 1) % 4))) {
					intersection[i][j] += 1;
				}
			}
		}
	} */

	update_intersection(origin, destination, true);
	if(a == 1) {
		wait_cars[origin] -= 1;
	}
	lock_release(traffic_lock);
}


  /* replace this default implementation with your own implementation */



/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */

	KASSERT(traffic_lock != NULL);
	lock_acquire(traffic_lock);
	
/*
	 for (unsigned int i = 0; i < 4; i++) {
                if (i != origin) {
                        for (unsigned int j = 0; j < 4; j++) {
                                if (!(i == destination && j == origin) && !(j != destination && (origin == (destination + 1) % 4 || i == (j + 1) % 4))) {
                                        intersection[i][j] -= 1;
                                }
                        }
                }
        }
*/
	update_intersection(origin, destination, false);	
	wake_cars(origin);
	lock_release(traffic_lock);


}
