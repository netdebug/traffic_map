#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/socket.h>/*PF_INET*/
#include <linux/netfilter_ipv4.h>/*NF_IP_PRE_FIRST*/
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/inet.h> /*in_aton()*/
#include <net/ip.h>
#include <net/tcp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("brytonlee01@gmail.com");

#ifndef NIPQUAD
#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]

#define NIPQUAD_FMT "%u.%u.%u.%u"
#endif

#define TCPHDR(skb) ((char*)(skb)->data+iph->ihl*4)

struct nf_hook_ops nf_pre_route; 
struct nf_hook_ops nf_out;

static void tcp_info(struct sk_buff *skb, struct iphdr *iph, struct tcphdr *tcph)
{
	int tot_len; /* packet data totoal length */
	int iph_len;
	int tcph_len;
	int tcp_offld_len;

	tot_len = ntohs(iph->tot_len);
	iph_len = ip_hdrlen(skb);
	tcph_len = tcph->doff * 4;
	tcp_offld_len = tot_len - ( iph_len + tcph_len );

	/* debug */
//	printk("total_len: %d, iph_len: %d, tcph_len: %d\n",
//		tot_len, iph_len, tcph_len);

	printk("tcp connection and src: "NIPQUAD_FMT":%d, dest: "NIPQUAD_FMT
			":%d, offload_length: %d\n",
		NIPQUAD(iph->saddr), ntohs(tcph->source),
		NIPQUAD(iph->daddr), ntohs(tcph->dest), tcp_offld_len);
}

/* output */
#if defined(LINUX_VERSION_CODE) && defined(KERNEL_VERSION) && \
	LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
unsigned int filter_out(unsigned int hooknum,
                        struct sk_buff *__skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{
#else
unsigned int filter_out(unsigned int hooknum,
                        struct sk_buff **__skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{
#endif
	struct sk_buff *skb;
	struct iphdr *iph;
	struct tcphdr *tcph;

#if defined(LINUX_VERSION_CODE) && defined(KERNEL_VERSION) && \
	LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
	skb = __skb;
#else 
	skb = *__skb;
#endif

	if(skb == NULL)
		return NF_ACCEPT;
	iph = ip_hdr(skb);
	if(iph == NULL)
		return NF_ACCEPT;

	if(iph->protocol == IPPROTO_TCP)
	{
		tcph=(struct tcphdr*)TCPHDR(skb);

		if ( tcph->source == htons(80) ) {
			tcp_info(skb, iph, tcph);		
		} 
	}
	
	return NF_ACCEPT;
}

/* input */
#if defined(LINUX_VERSION_CODE) && defined(KERNEL_VERSION) && \
	LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
unsigned int filter_pre_route(unsigned int hooknum,
                        struct sk_buff *__skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{
#else
unsigned int filter_pre_route(unsigned int hooknum,
                        struct sk_buff **__skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{
#endif
	struct sk_buff *skb;
	struct iphdr *iph;
	struct tcphdr *tcph;

#if defined(LINUX_VERSION_CODE) && defined(KERNEL_VERSION) && \
	LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
	skb = __skb;
#else 
	skb = *__skb;
#endif

	if(skb == NULL)
		return NF_ACCEPT;
	iph = ip_hdr(skb);
	if(iph == NULL)
		return NF_ACCEPT;

	if(iph->protocol == IPPROTO_TCP)
	{
		tcph=(struct tcphdr*)TCPHDR(skb);

		if ( tcph->dest == htons(80) ) {
			tcp_info(skb, iph, tcph);		
		} 

	}
	return NF_ACCEPT;
}

static int __init filter_init(void)
{
	int ret;

	/* input */
        nf_pre_route.hook = filter_pre_route;
        nf_pre_route.pf = AF_INET;
#if defined(LINUX_VERSION_CODE) && defined(KERNEL_VERSION) && \
	LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
        nf_pre_route.hooknum = NF_INET_PRE_ROUTING;
#else
	nf_pre_route.hooknum = NF_IP_PRE_ROUTING;
#endif
        nf_pre_route.priority = NF_IP_PRI_FIRST;
    
        ret = nf_register_hook(&nf_pre_route);
        if(ret < 0)
        {
            printk("%s\n", "can't modify skb hook!");
            return ret;
        }

	/* output */
	nf_out.hook = filter_out;
	nf_out.pf = AF_INET;
#if defined(LINUX_VERSION_CODE) && defined(KERNEL_VERSION) && \
	LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
	nf_out.hooknum = NF_INET_LOCAL_OUT;
#else
	nf_out.hooknum = NF_IP_LOCAL_OUT;
#endif 
	
	ret = nf_register_hook(&nf_out);
	if ( ret < 0 ) {
		printk("%s\n", "can't modify skb hook!");
		return ret;
	}

	printk("traffic_map used to monitor tcp connections traffic\n"
		"Author: brytonlee01@gmail.com\n");
	return 0;
}

static void filter_fini(void)
{
	nf_unregister_hook(&nf_pre_route);
	nf_unregister_hook(&nf_out);
	printk("[traffic_map]: bye...\n");
}
module_init(filter_init);
module_exit(filter_fini);

