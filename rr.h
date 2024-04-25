extern struct scheduler rr;

void init_rr(void);
void shutdown_rr(void);
void admit_rr(thread t);
void remove_rr(thread removing);
thread next_rr(void);
int qlen_rr(void);

