#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define max_name 64
#define max_murgi 32
#define max_history 64
#define max_queue 32

typedef enum
{
    FILE_NODE,
    DIR_NODE
} NodeType;

typedef struct Node
{
    char name[max_name];
    NodeType type;
    struct Node *baba;
    struct Node *murgiren[max_murgi];
    int murgi_count;
} Node;

typedef struct
{
    Node *stack[max_history];
    int top;
} NavStack;

typedef struct
{
    Node *queue[max_queue];
    int front, rear;
} BatchQueue;

Node *root;
Node *current;
NavStack nav_stack;
BatchQueue batch_queue;

Node *create_node(const char *name, NodeType type, Node *baba)
{
    Node *node = malloc(sizeof(Node));
    strncpy(node->name, name, max_name);
    node->type = type;
    node->baba = baba;
    node->murgi_count = 0;
    return node;
}

void add_murgi(Node *baba, Node *murgi)
{
    if (baba->murgi_count < max_murgi)
    {
        baba->murgiren[baba->murgi_count++] = murgi;
    }
}

void remove_murgi(Node *baba, int idx)
{
    for (int i = idx; i < baba->murgi_count - 1; ++i)
        baba->murgiren[i] = baba->murgiren[i + 1];
    baba->murgi_count--;
}

int find_murgi(Node *baba, const char *name)
{
    for (int i = 0; i < baba->murgi_count; ++i)
        if (strcmp(baba->murgiren[i]->name, name) == 0)
            return i;
    return -1;
}

void push_nav(Node *dir)
{
    if (nav_stack.top < max_history)
        nav_stack.stack[nav_stack.top++] = dir;
}

Node *pop_nav()
{
    if (nav_stack.top > 0)
        return nav_stack.stack[--nav_stack.top];
    return NULL;
}

void enqueue_batch(Node *file)
{
    if ((batch_queue.rear + 1) % max_queue != batch_queue.front)
    {
        batch_queue.queue[batch_queue.rear] = file;
        batch_queue.rear = (batch_queue.rear + 1) % max_queue;
    }
}

Node *dequeue_batch()
{
    if (batch_queue.front != batch_queue.rear)
    {
        Node *file = batch_queue.queue[batch_queue.front];
        batch_queue.front = (batch_queue.front + 1) % max_queue;
        return file;
    }
    return NULL;
}

void cmd_mkdir(const char *name)
{
    if (find_murgi(current, name) != -1)
    {
        printf("Folder already exists.\n");
        return;
    }
    Node *dir = create_node(name, DIR_NODE, current);
    add_murgi(current, dir);
}

void cmd_touch(const char *name)
{
    if (find_murgi(current, name) != -1)
    {
        printf("File already exists.\n");
        return;
    }
    Node *file = create_node(name, FILE_NODE, current);
    add_murgi(current, file);
}

void cmd_rm(const char *name)
{
    int idx = find_murgi(current, name);
    if (idx == -1)
    {
        printf("Not found.\n");
        return;
    }
    Node *node = current->murgiren[idx];
    if (node->type == DIR_NODE && node->murgi_count > 0)
    {
        printf("Directory not empty.\n");
        return;
    }
    free(node);
    remove_murgi(current, idx);
}

void cmd_cd(const char *name)
{
    if (strcmp(name, "..") == 0)
    {
        if (current->baba)
        {
            push_nav(current);
            current = current->baba;
        }
        return;
    }
    int idx = find_murgi(current, name);
    if (idx == -1 || current->murgiren[idx]->type != DIR_NODE)
    {
        printf("Directory not found.\n");
        return;
    }
    push_nav(current);
    current = current->murgiren[idx];
}

void cmd_ls()
{
    for (int i = 0; i < current->murgi_count; ++i)
    {
        Node *node = current->murgiren[i];
        printf("%s%s\n", node->name, node->type == DIR_NODE ? "/" : "");
    }
}

void cmd_search(const char *pattern) // bfs search korchi
{
    Node *queue[max_history];
    int front = 0, rear = 0;
    queue[rear++] = root;
    while (front < rear)
    {
        Node *node = queue[front++];
        if (strstr(node->name, pattern) != NULL)
            printf("%s%s\n", node->name, node->type == DIR_NODE ? "/" : "");
        for (int i = 0; i < node->murgi_count; ++i)
            queue[rear++] = node->murgiren[i];
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
    int idx = find_murgi(current, dest_name);
    if (idx == -1 || current->murgiren[idx]->type != DIR_NODE)
    {
        printf("Destination directory not found.\n");
        return;
    }
    Node *dest = current->murgiren[idx];
    while (batch_queue.front != batch_queue.rear)
    {
        Node *file = dequeue_batch();
        if (strcmp(action, "copy") == 0)
        {
            Node *copy = create_node(file->name, FILE_NODE, dest);
            add_murgi(dest, copy);
        }
        else if (strcmp(action, "move") == 0)
        {
            int fidx = find_murgi(file->baba, file->name);
            remove_murgi(file->baba, fidx);
            file->baba = dest;
            add_murgi(dest, file);
        }
    }
}

void cmd_enqueue(const char *name)
{
    int idx = find_murgi(current, name);
    if (idx == -1 || current->murgiren[idx]->type != FILE_NODE)
    {
        printf("File not found.\n");
        return;
    }
    enqueue_batch(current->murgiren[idx]);
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

    char cmd[128], cmd1[max_name], cmd2[max_name];
    while (1)
    {
        print_prompt();
        fgets(cmd, sizeof(cmd), stdin);
        if (sscanf(cmd, "mkdir %s", cmd1) == 1)
            cmd_mkdir(cmd1);
        else if (sscanf(cmd, "touch %s", cmd1) == 1)
            cmd_touch(cmd1);
        else if (sscanf(cmd, "rm %s", cmd1) == 1)
            cmd_rm(cmd1);
        else if (sscanf(cmd, "cd %s", cmd1) == 1)
            cmd_cd(cmd1);
        else if (strncmp(cmd, "ls", 2) == 0)
            cmd_ls();
        else if (sscanf(cmd, "search %s", cmd1) == 1)
            cmd_search(cmd1);
        else if (strncmp(cmd, "history", 7) == 0)
            cmd_history();
        else if (strncmp(cmd, "undo", 4) == 0)
            cmd_undo();
        else if (sscanf(cmd, "enqueue %s", cmd1) == 1)
            cmd_enqueue(cmd1);
        else if (sscanf(cmd, "batch %s %s", cmd1, cmd2) == 2)
            cmd_batch(cmd1, cmd2);
        else if (strncmp(cmd, "exit", 4) == 0)
            break;
        else
            printf("Unknown command.\n");
    }
    return 0;
}
