#include <unistd.h>    
#include <sys/types.h>  
#include <stdio.h>      
#include <stdlib.h>    
#include <pthread.h>  
#include <semaphore.h>

#define MAXIMUM_GUESTS 10
#define NUMBER_OF_ROOMS 5
#define false 0
#define true 1

/*
 * Declaring and initializing variables to get
 * the number of guests in swimming pool, number
 * of guests in fitness center, number of guests in
 * restaurant and the number of guests in Business Center
 */
int numberOfGuestsInSwimmingPool = 0;
int numberOfGuestsInFitnessCenter = 0;
int numberOfGuestsInRestaurant = 0;
int numberOfGuestsInBusinessCenter = 0;

// Variable to store the final balance for the guest
int totalAmountDue = 0;

/* 
 * Declaring array roomsAvailable to initialize the maximum number of rooms to 5
 * Declaring array roomsOccupied to check if a room is available for check-in or is it occupied by a guest
 * Initializing roomsOccupied to false as at first none of the 5 rooms are occupied by a guest
 */
int roomsAvailable[NUMBER_OF_ROOMS];
int roomsOccupied[NUMBER_OF_ROOMS] = {false, false, false, false, false};

/*
 * Global Variables:-
 * guest_CheckIn - To transfer the information of guest that checked-in from the guestMethod into the check-in method
 * guest_checkOut - To transfer the information of the guest that checked-out from the guestMethod into the check-out method
 */
int guest_CheckIn = 0;
int guest_CheckOut = 0;
int roomAvailableAfterCheckOut;
// Declaring semaphores
sem_t roomAssignment_mutex;                     // Semaphore to perform the mutual exclusion when assigning a room and checking out of a room

sem_t availableReservationist_CheckInCounter;   // Semaphore to check if a reservationist is available or occupied on the check-in counter
sem_t availableReservationist_CheckOutCounter;  // Semaphore to check if a reservationist is available or occupied on the check-out counter

sem_t greet_checkInReservationist;              // Semaphore to ensure the guest gets greeted after arriving at the check-in counter
sem_t greet_checkOutReservationist;             // Semaphore to ensure the guest gets greeted after arriving at the check-out counter

sem_t availableRoom;                            // Semaphore to check if a room is available to check-in and to ensure that not more than 5 rooms are assigned since there a max of 5 rooms
sem_t guest_getsRoom;                           // Semaphore to give the guest a room

sem_t calculateBalanceForGuest;                 // Semaphore for balance calculation for guest
sem_t guest_Payment;                            // Semaphore for guest payment

void *guestMethod(void *guestNumber)
{
	int guestNum;
	guestNum=*((int*)guestNumber);
	
	/* 
	 * Waits and checks if a room is available for check-in
	 * If available, proceeds to the following code, else
	 * waits for a guest to check-out so that the room is empty
	 */
	sem_wait(&availableRoom);
	printf("\nGuest %d waits for check-in\n", guestNum);

	// Waits to see if a reservationist is available at the check-in counter
	sem_wait(&availableReservationist_CheckInCounter);

	printf("Guest %d goes to the check-in reservationist\n", guestNum);

	// Assigns the value of the guest that checked in to the global variable guest_CheckIn to be used by the checkIn method
	guest_CheckIn = guestNum;

	// Signals that the check in reservationist has greeted the guest
	sem_post(&greet_checkInReservationist);
	
	// Waits to check if the guest has been assigned a room by the check-in reservationist or not
	sem_wait(&guest_getsRoom);
	printf("Guest %d receives room %d and completes check-in\n", guestNum, roomsAvailable[guestNum]);

	// Signals that the reservationist at check-in counter is now available and can serve another guest
	sem_post(&availableReservationist_CheckInCounter);
	

	// Randomly getting the task that the guest would do in the hotel
	int guestTask;
	guestTask = (rand() % 4) + 1;

	// Randomly getting the numbers between 1 to 3 for setting the sleep time
	int guestSleepTime;
	guestSleepTime = (rand() % 3) + 1;

	/* 
	 * if the value of guestTask = 1
	 * the guest goes to the swimming pool
	 * increments the value of variable numberOfGuestsInSwimmingPool
	 */
	if(guestTask == 1)
	{
		printf("Guest in swimming pool\n\n");
		sleep(guestSleepTime);
		numberOfGuestsInSwimmingPool++;
	}

	/* 
	 * if the value of guestTask = 2
	 * the guest goes to the restaurant
	 * increments the value of variable numberOfGuestsInSwimmingPool
	 */
	else if(guestTask == 2)
	{
		printf("Guest in restaurant\n\n");
		sleep(guestSleepTime);
		numberOfGuestsInRestaurant++;
	}

	/* 
	 * if the value of guestTask = 3
	 * the guest goes to the fitness center
	 * increments the value of variable numberOfGuestsInSwimmingPool
	 */
	else if(guestTask == 3)
	{
		printf("Guest in fitness center\n\n");
		sleep(guestSleepTime);
		numberOfGuestsInFitnessCenter++;
	}

	/* 
	 * if the value of guestTask = 4
	 * the guest goes to the business center
	 * increments the value of variable numberOfGuestsInSwimmingPool
	 */
	else if(guestTask == 4)
	{
		printf("Guest in business center\n\n");
		sleep(guestSleepTime);
		numberOfGuestsInBusinessCenter++;
	}

	// Waits and checks if a reservationist is available at the check-out counter
	sem_wait(&availableReservationist_CheckOutCounter);

	printf("\n----------------------------------------------------------\n");
	printf("\nGuest %d goes to the check-out reservationist and returns room %d\n", guestNum, roomsAvailable[guestNum]);

	// Assigns the value of the guest that checked out to the global variable guest_CheckOut to be used by the checkOut method
	guest_CheckOut = guestNum;

	// Signals that the reservationist at the check-out counter has greeted the guest and is ready to perform the check-out
	sem_post(&greet_checkOutReservationist);

	// Waits to check if the check-out counter has calculated the balance for the guest checking-out
	sem_wait(&calculateBalanceForGuest);
	printf("Guest %d receives the total balance of $%d\n", guestNum, totalAmountDue);
	printf("\nGuest %d makes a payment\n", guestNum);
	
	// Signals to inform the check-out counter that the payment has been made by the guest, and the check-out procedure can now be completed
	sem_post(&guest_Payment);
}

void *checkIn(void *checkInArgument)
{
	int checkIn;
	int rooms;
	int roomAssigned;
	//checkIn = *((int*)checkInArgument);

	for(checkIn = 0; checkIn < MAXIMUM_GUESTS; checkIn++)
	{
		/* 
		 * Waits to check if a guest has arrived at the check-in counter
		 * If the guest has arrived, the semaphore makes sure that the guest will first be 
		 * greeted at the check-in counter and then the check-in process will move forward
		 */
		sem_wait(&greet_checkInReservationist);
		printf("The check-in reservationist greets guest %d\n", guest_CheckIn);

		/*
		 * The loop checks if a room is available or not
		 * if a room is not occupied, the room could be assigned
		 * to the next available guest, and mark that room as occupied.
		 * Variable roomAssigned stores that value of the room that was assigned to the guest
		 * Mutual Exclusion is used for this as roomsOccupied and roomsAvailable are shared resources
		 * which are modified by the checkIn method as well as the checkOut method
		 */
		sem_wait(&roomAssignment_mutex);
		for(rooms = 0; rooms < NUMBER_OF_ROOMS; rooms++)
	    {
		  if(roomsOccupied[rooms] == false)
		    {
		      roomAssigned = rooms;
			  roomsOccupied[rooms] = true;
		      break;
		    }
		}

		// The value of the room assigned to the guest is marked at the specific index 
		// in the array based on the value of the guest that checked-in
		roomsAvailable[guest_CheckIn] = roomAssigned;
		printf("Assign room %d to guest %d\n", roomAssigned, guest_CheckIn);
		
		// Signals roomAssignment_mutex semaphore that the shared resource 
		// could now be accessed by someone else and is not in the critical section
		sem_post(&roomAssignment_mutex);
		
		// Signals that the guest has gotten the room, and the check-in process 
		// could now be continued further and completed
		sem_post(&guest_getsRoom);
	}
}

// Function to perform the check out of the guest from the hotel
void *checkOut(void *checkOutArgument)
{
	int checkOut;
	for(checkOut = 0; checkOut < MAXIMUM_GUESTS; checkOut++)
	{
		/* 
		 * Waits to check if a guest has arrived at the check-out counter
		 * If the guest has arrived, the semaphore makes sure that the guest will first be 
		 * greeted at the check-out counter and then the check-out process will move forward
		 */
		sem_wait(&greet_checkOutReservationist);
		printf("The check-out reservationist greets guest %d and receives the key from room %d\n", guest_CheckOut, roomsAvailable[guest_CheckOut]);
		printf("\nCalculate the balance for guest %d\n", guest_CheckOut);
		//roomsAvailable[guest_CheckOut] = guest_CheckOut;
		
		// Randomly calculates the total balance for guests. Keeping the minimum amount at 400
		totalAmountDue = (rand() % 1500) + 400;

		// Signals that the balance has been calculated for the guest checking out of the hotel, 
		// and the guest can thus, now make a payment
		sem_post(&calculateBalanceForGuest);

		// Waits to check if the guest checking out of the hotel has made a payment or not
		sem_wait(&guest_Payment);
		printf("Receive $%d from guest %d and complete the check-out\n", totalAmountDue, guest_CheckOut);
		printf("------------------------------------------------------------\n");
		
		// After completing the check-out, signals that the reservationist at the check-out counter
		// is now free and could server another guest wanting to check-out of the hotel
		sem_post(&availableReservationist_CheckOutCounter);

		// Assigns the value of the room that the guest checked out of to the variable roomAvailableAfterCheckOut
		roomAvailableAfterCheckOut = roomsAvailable[guest_CheckOut];
		/*
		 * Mutual Exclusion is used for this as roomsOccupied and roomsAvailable are shared resources
		 * which are modified by the checkIn method as well as the checkOut method
		 */
		// Waits while shared resource is in the critical section
		sem_wait(&roomAssignment_mutex);

		// After the check-out is completed, the room which was occupied is now marked 
		// empty and available for another guest to move into
		roomsOccupied[roomAvailableAfterCheckOut] = false;

		// Signals roomAssignment_mutex semaphore that the shared resource 
		// could now be accessed by someone else and is not in the critical section 
		sem_post(&roomAssignment_mutex);

		// Signals that a room is now empty and is available for another guest to check into
		sem_post(&availableRoom);
	}
}

// Main method to run the program
main()
{
	srand(time(NULL));
	int guests[10];
	int z;

	//Initializing semaphores
	sem_init(&availableReservationist_CheckInCounter, 0, 1);    // Initializing the value of semaphore availableReservationist_CheckInCounter to 1
	sem_init(&availableReservationist_CheckOutCounter, 0, 1);   // Initializing the value of semaphore availableReservationist_CheckOutCounter to 1

	sem_init(&greet_checkInReservationist, 0, 0);               // Initializing the value of semaphore greet_checkInReservationist to 0
	sem_init(&greet_checkOutReservationist, 0, 0);              // Initializing the value of semaphore greet_checkOutReservationist to 0

	sem_init(&availableRoom, 0, NUMBER_OF_ROOMS);               // Initializing the value of semaphore availableRoom to 5
	sem_init(&guest_getsRoom, 0, 0);                            // Initializing the value of semaphore guest_getsRoom to 0

	sem_init(&calculateBalanceForGuest, 0, 0);                  // Initializing the value of semaphore calculateBalanceForGuest to 0
	sem_init(&guest_Payment, 0, 0);                             // Initializing the value of semaphore guest_Payment to 0

	sem_init(&roomAssignment_mutex, 0, 1);                      // Initializing the value of semaphore roomAssignment_mutex to 1


	pthread_t guestThread[MAXIMUM_GUESTS]; // Declaring a thread array for the 10 guest threads
	pthread_t guestCheckInThread;          // Declaring a thread for the checkIn method
	pthread_t guestCheckOutThread;         // Declaring a thread for the checkOut method

	// Loop used for initializing the guest numbers
	for(z = 0; z < MAXIMUM_GUESTS; z++)
	{
		guests[z] = z+1;
	}

	
	// Creating the threads for the checkIn and the checkOut methods
	pthread_create(&guestCheckInThread, NULL, checkIn, NULL);
	pthread_create(&guestCheckOutThread, NULL, checkOut, NULL);

	//Loop for creating 10 guest threads to run parallely
	int i;
	for(i = 0; i < MAXIMUM_GUESTS; i++)
	{
		pthread_create(&guestThread[i], NULL, guestMethod, (void *) &guests[i]);
	}


	// Loop for joining the guest threads
	int j;
	for(j = 0; j < MAXIMUM_GUESTS; j++)
	{
		pthread_join(guestThread[j], NULL);
	}

	// Joining the checkIn and checkOut threads
	pthread_join(guestCheckInThread, NULL);
	pthread_join(guestCheckOutThread, NULL);

	// Destroy the semaphores
	sem_destroy(&availableReservationist_CheckInCounter);
	sem_destroy(&availableReservationist_CheckOutCounter);

	sem_destroy(&greet_checkInReservationist);
	sem_destroy(&greet_checkOutReservationist);

	sem_destroy(&availableRoom);
	sem_destroy(&roomAssignment_mutex);

	sem_destroy(&calculateBalanceForGuest);
	sem_destroy(&guest_Payment);
	

	// Print final results
	printf("\nTotal Guests: %d\n", MAXIMUM_GUESTS);
	printf("Pool: %d\n", numberOfGuestsInSwimmingPool);
	printf("Restaurant: %d\n", numberOfGuestsInRestaurant);
	printf("Fitness center: %d\n", numberOfGuestsInFitnessCenter);
	printf("Business center: %d\n", numberOfGuestsInBusinessCenter);
	pthread_exit(NULL);
}
