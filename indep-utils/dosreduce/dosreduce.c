#include<stdio.h>
#include<stdlib.h>
#include<errno.h>

#define INFINITY -1;
#define INC 0

int debug=0;

typedef struct node {
  int c;   /* color  0:white 1:grey 2:black */
  int d;   /* Distance */
  int p;   /* parent */
  int id;   
} node;

typedef struct QueueItem{
  struct node *u;
  struct QueueItem *next;
} QueueItem;

typedef struct ATlinks{
  int n1;
  int n2;
  struct ATlinks *next;
} ATlinks;

typedef struct CTinfo{
  int src;          
  int dst; 
  int *path;    
  int flag;   /* whether it should be used or not */
} CTinfo;

QueueItem *head, *tail;
ATlinks *AThead, *ATtail;

void enqueue(struct node *vertex );
struct node* dequeue(void);
void cleanq(void);


int main(int argc, char **argv)
{
  int i,j,k, maxd,id1,id2,noattackers,wt;
  int node_count,edges,victim;
  FILE *fp;
  struct node **V, *u;
  struct ATlinks *newlink;
  struct ATlinks *p;
  struct CTinfo **CrossTraffic;
  int **matrix;
  char temp[30];
  int pn1,pn2,*attackers;
  int found,activeconn,pathlen;
  int tempi;
  int *FT,lc,nc;


  if (argc < 3 ){
    printf("Usage: %s inet_topology #attackers #activeconnections\n",argv[0]);
    exit(0);
  }

  noattackers = atoi(argv[2]);
  activeconn = atoi(argv[3]);
  
  /* Read the node_count and edges for topo file */ 
  if ( (fp = fopen(argv[1],"r")) == NULL ){
    printf(" Error openning the adjanceny file\n");
    exit(1);
  }

  fscanf(fp,"%d %d",&node_count,&edges);
  /* Ignore location infomation */
  for(i=0;i<=node_count;i++){
    fgets(temp,30,fp);
  }

  /* Randomly choose victim node */
  victim = (int)((float)node_count*rand()/(RAND_MAX+1.0));
  if(debug){
    printf("node_count= %d edges %d victim=%d\n",node_count,edges,victim);
  }

  /* Initialise all remaining vertices 
   */
  V = (struct node **)malloc(node_count*sizeof(node));
  for(i=0;i<node_count;i++){
    V[i] = (struct node *) malloc (sizeof(node));
    V[i]->id=i;
    V[i]->c=0;
    V[i]->d=INFINITY;
    V[i]->p=INFINITY;
  }
  
  /* Initialise queue */
  head = tail = NULL;
  
  /* Initialise the root of bfs tree 
   * rootid can be the id of any node. like node 0 or leaf */
  V[victim]->c = 1;
  V[victim]->d = 0;
  V[victim]->p = INFINITY;

  enqueue(V[victim]);
  
  /* open file with adjaceny information */
  matrix = (int**)malloc(node_count*sizeof(int*));
  for(i=0;i<node_count;i++){
    matrix[i] = (int*)malloc(node_count*sizeof(int));
    for(j=0;j<node_count;j++){
      matrix[i][j]=0;
    }
  }

  for(i=0;i<edges;i++){
    fscanf(fp,"%d %d %d",&id1,&id2,&wt);
    matrix[id1][id2]=1;
    matrix[id2][id1]=1;
  }

  close(fp);

  
  while(head){
    u = (struct node *) malloc (sizeof(node));
    if(u== NULL){
      printf("Error allocationg u\n");
      exit(1);
    }
    u=dequeue();
    id1= u->id;
    for(i=0;i<node_count;i++){
      /* refer to adj matrix */
      if(matrix[id1][i] == 1 ){
	if (V[i]->c == 0){
	  V[i]->c =1;
	  V[i]->d = u->d + 1;
	  V[i]->p = u->id;
	  enqueue(V[i]);
	} 
      }
    }

      V[id1]->c = 2;
      V[id1]->p = u->p;
      V[id1]->d = u->d;
  }

  maxd =0;
  /* print out all the information */
  for (i=0;i<node_count;i++){
    /* printf("id: %d c=%d d=%d p=%d\n",V[i]->id,V[i]->c,V[i]->d,V[i]->p);  */
     if(maxd < V[i]->d)
       maxd = V[i]->d;

  }    
  if (debug) printf(" Max depth %d\n",maxd);
  
  /* Choose the attackers randomly in the topology  */
  attackers = (int*)malloc(noattackers*sizeof(int));
  for(i=0;i<noattackers;i++){
    attackers[i] = victim;
    /* Ensure the victim and attackers are distinct */
    while(attackers[i] == victim){
      attackers[i]= (int)((float)node_count*rand()/(RAND_MAX+1.0));
    }
    if(debug)
      printf("attacker[%d] = %d\n",i,attackers[i]);
  }

  /* Record the attack tree links */
  AThead = ATtail = NULL;
  for(i=0;i<noattackers;i++){
    pn1 = V[attackers[i]]->id;
    pn2 = V[attackers[i]]->p;
    while(pn2 != victim) {

      if(AThead == NULL){
	if ( (newlink = (struct ATlinks *)malloc(sizeof(ATlinks))) == NULL){
	  printf("Error allocationg space for new ATlink\n");
	  exit(1);
	}
	newlink->n1=pn1;
	newlink->n2=pn2;
	AThead = newlink;
	ATtail = newlink;
	newlink->next = NULL;
      } else {
	/* insert only if not already present */
	ATlinks *p = AThead;
	while (p){
	  if( (p->n1 != pn1) || (p->n2 != pn2) )
	    p = p->next; 
	  else 
	    break;
	}
	/* link not found */ 
	if(p==NULL){
	  	if ( (newlink = (struct ATlinks *)malloc(sizeof(ATlinks))) == NULL){
		  printf("Error allocationg space for new ATlink\n");
		  exit(1);
		}
		newlink->n1=pn1;
		newlink->n2=pn2;
		newlink->next = NULL;
		ATtail->next = newlink;
		ATtail = newlink;
	} else {
	  /* reached common attack tree. Break from while */
	  break; 
	}

      }
      pn1 = pn2;
      pn2 = V[pn1]->p;
    }
    /* Insert the last link before victim */ 
    if(pn2 == victim){
       if(AThead == NULL){
	if ( (newlink = (struct ATlinks *)malloc(sizeof(ATlinks))) == NULL){
	  printf("Error allocationg space for new ATlink\n");
	  exit(1);
	}
	newlink->n1=pn1;
	newlink->n2=pn2;
	AThead = newlink;
	ATtail = newlink;
	newlink->next = NULL;
      } else {
      /* insert only if not already present */
	ATlinks *p = AThead;
	while (p){
	  if( (p->n1 != pn1) || (p->n2 != pn2) )
	    p = p->next;
	  else 
	    break;
	}
      	/* link not found */ 
	if(p==NULL){
	  	if((newlink = (struct ATlinks *)malloc(sizeof(ATlinks))) == NULL){
		  printf("Error allocationg space for new ATlink\n");
		  exit(1);
		}
		newlink->n1=pn1;
		newlink->n2=pn2;
		newlink->next = NULL;
		ATtail->next = newlink;
		ATtail = newlink;
	}
      } /* end else */
    }
    
  }
	
  
  /* print the attack tree  and encode into matrix */
  i=0;
  p = AThead;
  while(p){
     if(debug) printf("%d n1= %d n2= %d\n",i++,p->n1,p->n2);
      matrix[p->n1][p->n2] = 2;
      p = p->next;
  }

  /* Now randomly choose the cross traffic source and sinks */
  CrossTraffic = (struct CTinfo **)malloc(activeconn*sizeof(CTinfo));
  for(k=0;k<activeconn;k++){
    if ((CrossTraffic[k] = (struct CTinfo *)malloc(sizeof(CTinfo))) == NULL){
      printf("%d:Error allocation Crosstraffic structure\n",errno);
      exit(1);
    }
    CrossTraffic[k]->src =(int)((float)node_count*rand()/(RAND_MAX+ 1.0));
    CrossTraffic[k]->dst =(int)((float)node_count*rand()/(RAND_MAX+ 1.0));
    if(CrossTraffic[k]->src == CrossTraffic[k]->dst){
      while(CrossTraffic[k]->dst == CrossTraffic[k]->src)
	CrossTraffic[k]->dst =(int)((float)node_count*rand()/(RAND_MAX+ 1.0));
    }
    CrossTraffic[k]->flag = 0; /* Usable */
    /* printf("src %d dst %d\n",CrossTraffic[k]->src, CrossTraffic[k]->dst);
       fflush(NULL); */
    
    /* fill path after bfs */
    /* Initialise the V matrix */
    for(i=0;i<node_count;i++){
      V[i]->id=i;
      V[i]->c=0;
      V[i]->d=INFINITY;
      V[i]->p=INFINITY;
    }
  
    /* Initialise queue */
    head = tail = NULL;
    tempi = CrossTraffic[k]->src;

    /* Initialise the root of bfs tree 
     * rootid can be the id of any node. like node 0 or leaf */
    V[tempi]->c = 1;
    V[tempi]->d = 0;
    V[tempi]->p = INFINITY;

    enqueue(V[tempi]);
    found=0;
    while(head){
      if((u = (struct node *)malloc(sizeof(node))) == NULL){
	printf("out of memory for u %d\n",errno);
	exit(1);
      }
	
      u=dequeue();
      id1= u->id;
      for(i=0;i<node_count;i++){
	/* refer to adj matrix */
	if(matrix[id1][i] != 0 ){
	  if (V[i]->c == 0){
	    V[i]->c =1;
	    V[i]->d = u->d + 1;
	    V[i]->p = u->id;
	    if (V[i]->id == CrossTraffic[k]->dst){
	      found = 1;
	      pathlen = V[i]->d;
	      break; 
	    }
	    enqueue(V[i]);
	  } 
	}
      }
      if(found == 1){ 
	cleanq();
	break;
      }
      V[id1]->c = 2;
      V[id1]->p = u->p;
      V[id1]->d = u->d;
    }

    /* print the information */
    if(debug)
      printf("%d Src %d Dst %d Path ",k,CrossTraffic[k]->src,CrossTraffic[k]->dst);

    /* Find the path from src to dst */
    /* Stored dst, dst-1...src */ 

    CrossTraffic[k]->path = (int*)malloc(sizeof(pathlen));
    for(j=pathlen;j>=0;j--){
      if(j==pathlen)
	CrossTraffic[k]->path[j] = CrossTraffic[k]->dst;
      else {
	tempi = CrossTraffic[k]->path[j+1];
	CrossTraffic[k]->path[j] = V[tempi]->p;
      }
      if(debug)
	printf("%d ",CrossTraffic[k]->path[j]);

    }
  
    /* Matrix encoding 
     * 0 = no link 
     * 1 = link ( no traffic )
     * 2 = attack link only 
     * 3 = attack + cross traffic link 
     * 4 = cross traffic link only but path on AT  
     * 5 = only one flow of cross traffic 
     * 6 = more than one flow of cross traffic on link 
     *

     * Flag encoding 
     * 0 : discard 
     * 1 : shared attack tree 
     * 2 : shared cross traffic links 
     **** */ 
    /* Check is flag should change */ 
    for(j=0;j<pathlen;j++){
      if((matrix[CrossTraffic[k]->path[j]][CrossTraffic[k]->path[j+1]] == 2) \
      || (matrix[CrossTraffic[k]->path[j]][CrossTraffic[k]->path[j+1]] == 3)){
	CrossTraffic[k]->flag = 1;
      }
    }

    for(j=0;j<pathlen;j++){
      switch(matrix[CrossTraffic[k]->path[j]][CrossTraffic[k]->path[j+1]]){
      case 1:
	if(CrossTraffic[k]->flag == 1)
	  tempi=4;
	else 
	  tempi=5;
	break;
      case 2:
	if(CrossTraffic[k]->flag == 0){
	  printf("Flag has to be 1 or 2 for %d\n",k);
	  printf("info: link %d %d, matrix = %d\n",\
		 CrossTraffic[k]->path[j],CrossTraffic[k]->path[j+1], matrix[CrossTraffic[k]->path[j]][CrossTraffic[k]->path[j+1]]); 
	  exit(1);
	}
	tempi=3;
	break;
      case 3:
	if(CrossTraffic[k]->flag == 0){
	  printf("Flag has to be 1 or 2 for %d\n",k);
  	  printf("info: link %d %d, matrix = %d\n",\
		 CrossTraffic[k]->path[j],CrossTraffic[k]->path[j+1], matrix[CrossTraffic[k]->path[j]][CrossTraffic[k]->path[j+1]]); 

	  exit(1);
	}
	tempi=3;
	break;
	/*
       //case 4:
	//if(CrossTraffic[k]->flag == 1)
	 // tempi=4;
	//else {
	//  tempi=6;
	//  CrossTraffic[k]->flag = 2;
	//}
	//break;
      //case 5:
//	if (CrossTraffic[k]->flag == 0)
//	  tempi = 5;
//	else {
//	  CrossTraffic[k]->flag =2;
//	  tempi = 6;
//	}
//	break; 
	*/
	
}
      matrix[CrossTraffic[k]->path[j]][CrossTraffic[k]->path[j+1]] = tempi;
    } /* for the switch */ 

    if(debug) printf("Flag %d\n",CrossTraffic[k]->flag);
  } /* Close for number of active connections  */

  /* Get number of nodes and links in reducted topology */ 
  FT = (int *)malloc(node_count*sizeof(int));
  if ( FT == NULL){
    printf("out of memory\n");
    exit(0);
  }
  for(i=0;i<node_count;i++)
    FT[i] =0;
  lc=0;
  nc=0;

  for(i=0;i<node_count;i++){
    for(j=0;j<node_count;j++){
      // printf("[%d][%d]= %d\n",i,j,matrix[i][j]);
      if((matrix[i][j] == 2) || (matrix[i][j] == 3) || (matrix[i][j] == 4) || (matrix[i][j] == 6)){
	if(FT[i]==0){
	  FT[i] = 1;
	  printf("node %d\n",i);
	  nc++;
	}
	if(FT[j] == 0){
	  FT[j]=1;
  	  printf("node %d\n",j);
	  nc++;
	}
	printf("link %d %d\n",i,j);
	lc++;
      } 
    }
  }
  printf("\nSummary\n");
  printf("Total: Nodes %d Links %d\n",nc,lc);
  printf("Victim: %d\n",victim);
  printf("Attackers:");
  for(i=0;i<noattackers;i++) printf(" %d",attackers[i]);
  printf("\n");
  
  return 0; 
}
  
  

void cleanq(void)
{
  struct node *p;
  while(head)
    p = dequeue();
}

void enqueue(struct node *vertex)
{
  QueueItem *p = (struct QueueItem *) malloc (sizeof(QueueItem));
  if ( p == NULL){
    printf("Error here");
    exit(0);
  }
  p->u = vertex;
  p->next = NULL;

  if(head==NULL){
    head = p;
    tail = p;
  }
  else {
    tail->next= p;
    tail = p;
  }
}

struct node* dequeue(void)
{
  struct node *retvertex;
  QueueItem *p = (struct QueueItem *) malloc (sizeof(QueueItem));

  if (head == tail) {
    retvertex = head->u;
    head = tail = NULL;
  } else {
    p = head;
    retvertex=head->u;
    head = head->next;
    free(p);
  }
  return retvertex;
}

