/*
 * patricia_test.c
 * Patricia Trie test code.
 *
 * Compiling the test code (or any other file using libpatricia):
 *
 *     gcc -g -Wall -I. -L. -o ptest patricia_test.c -lpatricia
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <rpc/rpc.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "patricia.h"

/*
 * Arbitrary data associated with a given node.
 */
struct Node {
  int    foo;
  double bar;
};

typedef struct{
    struct in_addr ip;
    unsigned int mask;
} cidr_t;


int main(int argc, char **argv) {
  struct ptree *phead;
  struct ptree *p, *pfind;
  struct ptree_mask *pm;
  FILE *fp;
  char line[128];
  //char addr_str[16];

  if (argc < 2) {
    printf("Usage: %s <TCP stream>\n", argv[0]);
    exit(-1);
  }

  /*
   * Open file of IP addresses and masks.
   * Each line looks like:
   *    10.0.3.4 0xffff0000
   */
  if ((fp = fopen(argv[1], "r")) == NULL) {
    printf("File %s doesn't seem to exist\n", argv[1]);
    exit(1);
  }

  /*
   * Initialize the Patricia trie by doing the following:
   *   1. Assign the head pointer a default route/default node
   *   2. Give it an address of 0.0.0.0 and a mask of 0x00000000
   *      (matches everything)
   *   3. Set the bit position (p_b) to 0.
   *   4. Set the number of masks to 1 (the default one).
   *   5. Point the head's 'left' and 'right' pointers to itself.
   * NOTE: This should go into an intialization function.
   */
  phead = (struct ptree *)malloc(sizeof(struct ptree));

  if (!phead) {
    perror("Allocating p-trie node");
    exit(1);
  }

  bzero(phead, sizeof(*phead));
  phead->p_m = (struct ptree_mask *)malloc(sizeof(struct ptree_mask));

  if (!phead->p_m) {
    perror("Allocating p-trie mask data");
    exit(1);
  }

  bzero(phead->p_m, sizeof(*phead->p_m));
  pm = phead->p_m;
  pm->pm_data = (struct Node *)malloc(sizeof(struct Node));

  if (!pm->pm_data) {
    perror("Allocating p-trie mask's node data");
    exit(1);
  }

  bzero(pm->pm_data, sizeof(*pm->pm_data));
  /*******
   *
   * Fill in default route/default node data here.
   *
   *******/
  phead->p_mlen = 1;
  phead->p_left = phead->p_right = phead;

  /*
   * The main loop to insert nodes.
   */
  char ip_str[20];
  unsigned int mask;
  cidr_t cidr;
  unsigned int h_addr;

  while (fgets(line, 128, fp)) {
    /*
     * Read in each IP address and mask and convert them to
     * more usable formats.
     */
    sscanf(line, "%[^/]%*c%d", ip_str, &mask);
    printf("ip_str:%s mask:%d\n", ip_str, mask);
    inet_aton(ip_str, &cidr.ip);
    cidr.mask = ~0 << (32 - mask);
    struct in_addr addr = cidr.ip;
    printf("addr is %d mask:0x%x\n", addr.s_addr, cidr.mask);

    /*
     * Create a Patricia trie node to insert.
     */
    p = (struct ptree *)malloc(sizeof(struct ptree));

    if (!p) {
      perror("Allocating p-trie node");
      exit(1);
    }

    bzero(p, sizeof(*p));

    /*
     * Allocate the mask data.
     */
    p->p_m = (struct ptree_mask *)malloc(sizeof(struct ptree_mask));

    if (!p->p_m) {
      perror("Allocating p-trie mask data");
      exit(1);
    }

    bzero(p->p_m, sizeof(*p->p_m));

    /*
     * Allocate the data for this node.
     * Replace 'struct Node' with whatever you'd like.
     */
    pm = p->p_m;
    pm->pm_data = (struct Node *)malloc(sizeof(struct Node));

    if (!pm->pm_data) {
      perror("Allocating p-trie mask's node data");
      exit(1);
    }

    bzero(pm->pm_data, sizeof(*pm->pm_data));

    /*
     * Assign a value to the IP address and mask field for this
     * node.
     */
    h_addr = ntohl(addr.s_addr);
    p->p_key = h_addr; /* host byte order */
    p->p_m->pm_mask = cidr.mask;
    //p->p_m->pm_mask = htonl(cidr.mask);
    pfind = pat_search(h_addr, phead);

    printf("key %d addr %d pm_mask 0x%x\n",p->p_key, h_addr, p->p_m->pm_mask);
    printf("pfind 0x%x key:%d mask:0x%x\n",pfind,  pfind->p_key, pfind->p_m->pm_mask);
    //if (pfind->p_key==(addr.s_addr&pfind->p_m->pm_mask)) {
    if (pfind->p_key != 0 && (pfind->p_key&pfind->p_m->pm_mask) == (h_addr & pfind->p_m->pm_mask)) {
      printf("key:%d find key:%d after mask:%d\n", addr.s_addr, pfind->p_key, h_addr & pfind->p_m->pm_mask);
      printf("Found.\n");
    } else {

     /*
      * Insert the node.
      * Returns the node it inserted on success, 0 on failure.
      */
      printf("%d: ", h_addr);
      printf("Inserted.\n");
      p = pat_insert(p, phead);
    }

    if (!p) {
      fprintf(stderr, "Failed on pat_insert\n");
      exit(1);
    }
  }

  exit(0);
}
