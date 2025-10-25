#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LEN 50
#define CLASS_LEN 10
#define MAX_SEATS 10
#define MAX_STACK 10

/* Reservation linked list node */
typedef struct Reservation
{
    long pnr;
    char name[NAME_LEN];
    int age;
    char seat_class[CLASS_LEN];
    int seat_no;
    struct Reservation *next;
} Reservation;

/* Queue node for waiting list */
typedef struct WaitingNode
{
    char name[NAME_LEN];
    int age;
    struct WaitingNode *next;
} WaitingNode;

/* Stack for cancellation history */
typedef struct CancelRecord
{
    long pnr;
    char name[NAME_LEN];
} CancelRecord;

/* Flight node for BST */
typedef struct Flight
{
    int flight_id;
    char source[30];
    char destination[30];
    char time[20];
    int capacity;
    int seats_booked;
    int seats[MAX_SEATS]; // 0=empty, 1=booked
    Reservation *res_head;
    WaitingNode *wait_front, *wait_rear;
    struct Flight *left, *right; // for BST
} Flight;

Flight *root = NULL; // BST root for flights
long next_pnr = 1000;
CancelRecord cancel_stack[MAX_STACK];
int stack_top = -1;

void push_cancel(long pnr, const char *name)
{
    if (stack_top == MAX_STACK - 1)
    {
        printf("Cancel history full!\n");
        return;
    }
    stack_top++;
    cancel_stack[stack_top].pnr = pnr;
    strcpy(cancel_stack[stack_top].name, name);
}
void pop_cancel()
{
    if (stack_top < 0)
    {
        printf("No recent cancellations to undo.\n");
        return;
    }
    printf("Undo cancellation: PNR %ld (%s)\n",
           cancel_stack[stack_top].pnr, cancel_stack[stack_top].name);
    stack_top--;
}

void enqueue_wait(Flight *f, const char *name, int age)
{
    WaitingNode *w = malloc(sizeof(WaitingNode));
    strcpy(w->name, name);
    w->age = age;
    w->next = NULL;
    if (!f->wait_rear)
        f->wait_front = f->wait_rear = w;
    else
    {
        f->wait_rear->next = w;
        f->wait_rear = w;
    }
}
WaitingNode *dequeue_wait(Flight *f)
{
    if (!f->wait_front)
        return NULL;
    WaitingNode *tmp = f->wait_front;
    f->wait_front = tmp->next;
    if (!f->wait_front)
        f->wait_rear = NULL;
    return tmp;
}

Flight *create_flight(int id, const char *src, const char *dest, const char *time, int cap)
{
    Flight *f = malloc(sizeof(Flight));
    f->flight_id = id;
    strcpy(f->source, src);
    strcpy(f->destination, dest);
    strcpy(f->time, time);
    f->capacity = cap;
    f->seats_booked = 0;
    f->res_head = NULL;
    f->wait_front = f->wait_rear = NULL;
    f->left = f->right = NULL;
    for (int i = 0; i < cap; i++)
        f->seats[i] = 0;
    return f;
}
Flight *insert_flight(Flight *root, Flight *f)
{
    if (!root)
        return f;
    if (f->flight_id < root->flight_id)
        root->left = insert_flight(root->left, f);
    else
        root->right = insert_flight(root->right, f);
    return root;
}
Flight *search_flight(Flight *root, int id)
{
    if (!root || root->flight_id == id)
        return root;
    if (id < root->flight_id)
        return search_flight(root->left, id);
    return search_flight(root->right, id);
}
void inorder_flights(Flight *root)
{
    if (!root)
        return;
    inorder_flights(root->left);
    printf("Flight %d: %s -> %s at %s | Seats: %d/%d\n",
           root->flight_id, root->source, root->destination, root->time,
           root->seats_booked, root->capacity);
    inorder_flights(root->right);
}

void book_ticket(Flight *f, const char *name, int age, const char *cls)
{
    if (f->seats_booked >= f->capacity)
    {
        printf("Flight full! Adding to waiting list...\n");
        enqueue_wait(f, name, age);
        return;
    }

    Reservation *r = malloc(sizeof(Reservation));
    r->pnr = next_pnr++;
    strcpy(r->name, name);
    r->age = age;
    strcpy(r->seat_class, cls);

    // Assign first available seat
    for (int i = 0; i < f->capacity; i++)
    {
        if (f->seats[i] == 0)
        {
            r->seat_no = i + 1;
            f->seats[i] = 1;
            break;
        }
    }

    // Insert at head (linked list)
    r->next = f->res_head;
    f->res_head = r;
    f->seats_booked++;

    printf("Booked! PNR %ld | Name: %s | Seat %d | Flight %d\n", r->pnr, r->name, r->seat_no, f->flight_id);
}

/* Cancel ticket */
void cancel_ticket(Flight *f, long pnr)
{
    Reservation *prev = NULL, *cur = f->res_head;
    while (cur)
    {
        if (cur->pnr == pnr)
        {
            if (prev)
                prev->next = cur->next;
            else
                f->res_head = cur->next;
            f->seats[cur->seat_no - 1] = 0;
            f->seats_booked--;
            printf("Cancelled: PNR %ld (%s)\n", cur->pnr, cur->name);
            push_cancel(cur->pnr, cur->name);
            free(cur);

            // Assign seat to waiting passenger
            WaitingNode *w = dequeue_wait(f);
            if (w)
            {
                printf("Assigning seat to waiting passenger %s\n", w->name);
                book_ticket(f, w->name, w->age, "Economy");
                free(w);
            }
            return;
        }
        prev = cur;
        cur = cur->next;
    }
    printf("PNR not found.\n");
}

/* Display passengers */
void display_passengers(Flight *f)
{
    printf("Passengers for Flight %d (%s -> %s):\n", f->flight_id, f->source, f->destination);
    Reservation *r = f->res_head;
    while (r)
    {
        printf("PNR %ld | Name: %s | Seat %d | Class %s\n",
               r->pnr, r->name, r->seat_no, r->seat_class);
        r = r->next;
    }
}

int main()
{
    int choice;
    // Pre-add few flights
    root = insert_flight(root, create_flight(101, "Mumbai", "Delhi", "08:00", 3));
    root = insert_flight(root, create_flight(102, "Chennai", "Hyderabad", "09:30", 4));
    root = insert_flight(root, create_flight(103, "Pune", "Bangalore", "11:00", 5));

    do
    {
        printf("\n===== AIRLINE SYSTEM (DS Version) =====\n");
        printf("1. View All Flights (Inorder BST)\n");
        printf("2. Book Ticket\n");
        printf("3. Cancel Ticket\n");
        printf("4. View Passengers (Flight)\n");
        printf("5. Undo Last Cancel (Stack)\n");
        printf("6. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        if (choice == 1)
        {
            inorder_flights(root);
        }
        else if (choice == 2)
        {
            int id, age;
            char name[NAME_LEN], cls[CLASS_LEN];
            printf("Enter Flight ID: ");
            scanf("%d", &id);
            Flight *f = search_flight(root, id);
            if (!f)
            {
                printf("Flight not found!\n");
                continue;
            }
            printf("Enter Name: ");
            scanf("%s", name);
            printf("Enter Age: ");
            scanf("%d", &age);
            printf("Enter Class: ");
            scanf("%s", cls);
            book_ticket(f, name, age, cls);
        }
        else if (choice == 3)
        {
            int id;
            long pnr;
            printf("Enter Flight ID: ");
            scanf("%d", &id);
            Flight *f = search_flight(root, id);
            if (!f)
            {
                printf("Flight not found!\n");
                continue;
            }
            printf("Enter PNR: ");
            scanf("%ld", &pnr);
            cancel_ticket(f, pnr);
        }
        else if (choice == 4)
        {
            int id;
            printf("Enter Flight ID: ");
            scanf("%d", &id);
            Flight *f = search_flight(root, id);
            if (f)
                display_passengers(f);
            else
                printf("Flight not found!\n");
        }
        else if (choice == 5)
        {
            pop_cancel();
        }
        else if (choice != 6)
        {
            printf("Invalid choice!\n");
        }
    } while (choice != 6);

    printf("Exiting system...\n");
    return 0;
}