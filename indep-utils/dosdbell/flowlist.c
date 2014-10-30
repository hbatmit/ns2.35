#include "flowlist.h"
#include<stdlib.h>
#include<stdio.h>

struct flow_list *create_new_flow_event(double time, double bw){  
    struct flow_list *new_node;
    new_node = (struct flow_list*)malloc(sizeof(struct flow_list));
    new_node->time = time;
    new_node->bw = bw;
    new_node->next = NULL;
    return(new_node);
}



void flow_list_insert(double time,double bw, struct flow_list **root){
struct flow_list* node;
struct flow_list *new_node;

  if(*root==NULL){
    new_node = create_new_flow_event(time,bw);
    *root = new_node;
  }
  else if((*root)->time > time){
    new_node = create_new_flow_event(time,bw);
    new_node->next = (*root);
    *root = new_node;
  }
  else if((*root)->time == time){
    (*root)->bw+=bw;
  }
  else{
    for(node = (*root); node->next != NULL && node->next->time < time ;node = node->next);    
    
    if(node->next == NULL){
      new_node = create_new_flow_event(time,bw);
      node->next = new_node;
    }
    else if(node->next->time==time){
      node->next->bw += bw;
    }
    else{
      new_node = create_new_flow_event(time,bw);
      new_node->next=node->next;
      node->next = new_node;
    }
  }
return;
}


void dump_flow_list_to_file(FILE* file, struct flow_list* root){
  struct flow_list *node;
  double total_bw = 0;
  for(node=root;node!=NULL;node=node->next){
    total_bw+=node->bw;
    fprintf(file,"%lf %lf %lf\n",node->time,node->bw,total_bw);
  }
}


int get_no_events(struct flow_list *root){
  int NoNodes;
  struct flow_list *node;
  for(node=root,NoNodes=0;node!=NULL;node=node->next,NoNodes++);
  return(NoNodes);
}

struct flow_event* get_all_events(struct flow_list *root){
  int NoNodes=0;
  int i;
  struct flow_event *allEvents;
  struct flow_list *node;

  NoNodes = get_no_events(root);

  allEvents = (struct flow_event*)malloc(NoNodes*sizeof(struct flow_event));
  for(node=root,i=0;node!=NULL;node=node->next,i++){
    allEvents[i].delay = node->time;
    allEvents[i].deltaCBR = node->bw;
    if(i==0)
      allEvents[i].NetCBR = node->bw;
    else
      allEvents[i].NetCBR = allEvents[i-1].NetCBR + node->bw;
  }
  return(allEvents);
}








