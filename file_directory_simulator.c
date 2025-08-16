#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 64
#define MAX_CHILDREN 32
#define MAX_HISTORY 64
#define MAX_QUEUE 32

typedef enum
{
    FILE_NODE,
    DIR_NODE
} NodeType;

typedef struct Node
{
    char name[MAX_NAME];
    NodeType type;
    struct Node *parent;
    struct Node *children[MAX_CHILDREN];
    int child_count;
} Node;

// Navigation history stack
typedef struct
{
    Node *stack[MAX_HISTORY];
    int top;
} NavStack;

// Batch queue for copy/move
typedef struct
{
    Node *queue[MAX_QUEUE];
    int front, rear;
} BatchQueue;

// Root directory
Node *root;
Node *current;
NavStack nav_stack;
BatchQueue batch_queue;

// Utility functions
Node *create_node(const char *name, NodeType type, Node *parent)
{
    Node *node = malloc(sizeof(Node));
    strncpy(node->name, name, MAX_NAME);
    node->type = type;
    node->parent = parent;
    node->child_count = 0;
    return node;
}

void add_child(Node *parent, Node *child)
{
    if (parent->child_count < MAX_CHILDREN)
    {
        parent->children[parent->child_count++] = child;
    }
}

void remove_child(Node *parent, int idx)
{
    for (int i = idx; i < parent->child_count - 1; ++i)
        parent->children[i] = parent->children[i + 1];
    parent->child_count--;
}

int find_child(Node *parent, const char *name)
{
    for (int i = 0; i < parent->child_count; ++i)
        if (strcmp(parent->children[i]->name, name) == 0)
            return i;
    return -1;
}

// Navigation stack functions
void push_nav(Node *dir)
{
    if (nav_stack.top < MAX_HISTORY)
        nav_stack.stack[nav_stack.top++] = dir;
}

Node *pop_nav()
{
    if (nav_stack.top > 0)
        return nav_stack.stack[--nav_stack.top];
    return NULL;
}

// Batch queue functions
void enqueue_batch(Node *file)
{
    if ((batch_queue.rear + 1) % MAX_QUEUE != batch_queue.front)
    {
        batch_queue.queue[batch_queue.rear] = file;
        batch_queue.rear = (batch_queue.rear + 1) % MAX_QUEUE;
    }
}

Node *dequeue_batch()
{
    if (batch_queue.front != batch_queue.rear)
    {
        Node *file = batch_queue.queue[batch_queue.front];
        batch_queue.front = (batch_queue.front + 1) % MAX_QUEUE;
        return file;
    }
    return NULL;
}

// Command implementations
void cmd_mkdir(const char *name)
{
    if (find_child(current, name) != -1)
    {
        printf("Folder already exists.\n");
        return;
    }
    Node *dir = create_node(name, DIR_NODE, current);
    add_child(current, dir);
}

void cmd_touch(const char *name)
{
    if (find_child(current, name) != -1)
    {
        printf("File already exists.\n");
        return;
    }
    Node *file = create_node(name, FILE_NODE, current);
    add_child(current, file);
}

void cmd_rm(const char *name)
{
    int idx = find_child(current, name);
    if (idx == -1)
    {
        printf("Not found.\n");
        return;
    }
    Node *node = current->children[idx];
    if (node->type == DIR_NODE && node->child_count > 0)
    {
        printf("Directory not empty.\n");
        return;
    }
    free(node);
    remove_child(current, idx);
}

void cmd_cd(const char *name)
{
    if (strcmp(name, "..") == 0)
    {
        if (current->parent)
        {
            push_nav(current);
            current = current->parent;
        }
        return;
    }
    int idx = find_child(current, name);
    if (idx == -1 || current->children[idx]->type != DIR_NODE)
    {
        printf("Directory not found.\n");
        return;
    }
    push_nav(current);
    current = current->children[idx];
}

void cmd_ls()
{
    for (int i = 0; i < current->child_count; ++i)
    {
        Node *node = current->children[i];
        printf("%s%s\n", node->name, node->type == DIR_NODE ? "/" : "");
    }
}

void cmd_search(const char *pattern)
{
    // BFS search
    Node *queue[MAX_HISTORY];
    int front = 0, rear = 0;
    queue[rear++] = root;
    while (front < rear)
    {
        Node *node = queue[front++];
        if (strstr(node->name, pattern) != NULL)
            printf("%s%s\n", node->name, node->type == DIR_NODE ? "/" : "");
        for (int i = 0; i < node->child_count; ++i)
            queue[rear++] = node->children[i];
    }
}

void cmd_history()
{
    printf("Navigation history:\n");
    for (int i = 0; i < nav_stack.top; ++i)
        printf("%s/\n", nav_stack.stack[i]->name);
}

void cmd_undo()
{
    Node *prev = pop_nav();
    if (prev)
        current = prev;
    else
        printf("No previous directory.\n");
}

void cmd_batch(const char *action, const char *dest_name)
{
    int idx = find_child(current, dest_name);
    if (idx == -1 || current->children[idx]->type != DIR_NODE)
    {
        printf("Destination directory not found.\n");
        return;
    }
    Node *dest = current->children[idx];
    while (batch_queue.front != batch_queue.rear)
    {
        Node *file = dequeue_batch();
        if (strcmp(action, "copy") == 0)
        {
            Node *copy = create_node(file->name, FILE_NODE, dest);
            add_child(dest, copy);
        }
        else if (strcmp(action, "move") == 0)
        {
            // Remove from current parent
            int fidx = find_child(file->parent, file->name);
            remove_child(file->parent, fidx);
            file->parent = dest;
            add_child(dest, file);
        }
    }
}

void cmd_enqueue(const char *name)
{
    int idx = find_child(current, name);
    if (idx == -1 || current->children[idx]->type != FILE_NODE)
    {
        printf("File not found.\n");
        return;
    }
    enqueue_batch(current->children[idx]);
}

void print_prompt()
{
    printf("[%s]$ ", current->name);
}

int main()
{
    root = create_node("root", DIR_NODE, NULL);
    current = root;
    nav_stack.top = 0;
    batch_queue.front = batch_queue.rear = 0;

    char cmd[128], arg1[MAX_NAME], arg2[MAX_NAME];
    while (1)
    {
        print_prompt();
        fgets(cmd, sizeof(cmd), stdin);
        if (sscanf(cmd, "mkdir %s", arg1) == 1)
            cmd_mkdir(arg1);
        else if (sscanf(cmd, "touch %s", arg1) == 1)
            cmd_touch(arg1);
        else if (sscanf(cmd, "rm %s", arg1) == 1)
            cmd_rm(arg1);
        else if (sscanf(cmd, "cd %s", arg1) == 1)
            cmd_cd(arg1);
        else if (strncmp(cmd, "ls", 2) == 0)
            cmd_ls();
        else if (sscanf(cmd, "search %s", arg1) == 1)
            cmd_search(arg1);
        else if (strncmp(cmd, "history", 7) == 0)
            cmd_history();
        else if (strncmp(cmd, "undo", 4) == 0)
            cmd_undo();
        else if (sscanf(cmd, "enqueue %s", arg1) == 1)
            cmd_enqueue(arg1);
        else if (sscanf(cmd, "batch %s %s", arg1, arg2) == 2)
            cmd_batch(arg1, arg2);
        else if (strncmp(cmd, "exit", 4) == 0)
            break;
        else
            printf("Unknown command.\n");
    }
    return 0;
}