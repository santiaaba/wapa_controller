typedef enum { S_ONLINE, S_OFFLINE} T_site_status;

/*****************************
 	Sitio
******************************/
struct list_worker;
typedef struct {
	unsigned long int id;
	unsigned long int version;
	char name[100];
	T_site_status status;
	struct list_worker *workers;
} T_site;

void site_init(T_site *s);

unsigned long int site_get_id(T_site *s);
unsigned long int site_get_version(T_site *s);
void site_get_name(T_site *s);
T_site_status site_get_status(T_site *s);

/*****************************
 	Worker
******************************/
typedef T_list_site;
typedef struct {
	char name[100];
	char ip[15];
	T_list_site sites;
} T_worker;

void worker_init(T_worker *w);

/*****************************
 	Proxy
******************************/
typedef struct {
	char name[100];
	char ip[15];
} T_proxy;

/*****************************
 	Lista de Workers
******************************/
typedef struct w_node {
	T_worker *data;
	struct w_node *next;
} list_w_node;

typedef struct list_worker{
	unsigned int size;
	list_w_node *first;
	list_w_node *last;
	list_w_node *actual;
} T_list_worker;

void list_worker_init(T_list_worker *l);
void list_worker_add(T_list_worker *l, T_worker *w);
T_worker ^list_worker_get(T_list_worker *l);
unsigned int list_worker_size(T_list_worker *l);
int list_worker_eol(T_list_worker *l);
int list_worker_remove(T_list_worker *l);
void list_worker_destroy(T_list_worker *l);

/*****************************
 	Lista de Sitios
******************************/
typedef struct s_node {
	T_site *data;
	struct s_node *next;
} list_s_node;

typedef struct {
	unsigned int size;
	list_s_node *first;
	list_s_node *actual;
} T_list_site;

void list_site_init(T_list_site *l);
void list_site_add(T_list_site *l, T_site *s);
T_site ^list_site_get(T_list_site *l);
unsigned int list_site_size(T_list_site *l);
int list_site_eol(T_list_site *l);
int list_site_remove(T_list_site *l);
void list_site_destroy(T_list_site *l);

/*****************************
 	Lista de Proxys
******************************/
typedef struct s_node {
	T_proxy *data;
	struct s_node *next;
} list_p_node;

typedef struct {
	unsigned int size;
	list_p_node *first;
	list_p_node *actual;
} T_list_proxy;

void list_proxy_init(T_list_proxy *l);
void list_proxy_add(T_list_proxy *l, T_proxy *s);
T_proxy ^list_proxy_get(T_list_proxy *l);
unsigned int list_proxy_size(T_list_proxy *l);
int list_proxy_eol(T_list_proxy *l);
int list_proxy_remove(T_list_proxy *l);
void list_proxy_destroy(T_list_proxy *l);
