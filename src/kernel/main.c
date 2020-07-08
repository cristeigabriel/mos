#include <include/mman.h>
#include <include/msgui.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cpu/exception.h"
#include "cpu/gdt.h"
#include "cpu/hal.h"
#include "cpu/idt.h"
#include "cpu/pit.h"
#include "cpu/tss.h"
#include "devices/ata.h"
#include "devices/char/memory.h"
#include "devices/kybrd.h"
#include "devices/mouse.h"
#include "devices/pci.h"
#include "devices/serial.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "ipc/message_queue.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "multiboot2.h"
#include "net/devices/rtl8139.h"
#include "net/dhcp.h"
#include "net/dns.h"
#include "net/icmp.h"
#include "net/net.h"
#include "net/tcp.h"
#include "proc/task.h"
#include "system/framebuffer.h"
#include "system/sysapi.h"
#include "system/time.h"
#include "system/timer.h"
#include "system/uiserver.h"
#include "utils/math.h"
#include "utils/printf.h"
#include "utils/string.h"

extern struct thread *current_thread;
extern struct vfs_file_system_type ext2_fs_type;

void setup_window_server(struct Elf32_Layout *elf_layout)
{
	uiserver_init(current_thread);
	mq_open(WINDOW_SERVER_SHM, 0);

	struct framebuffer *fb = get_framebuffer();
	uint32_t screen_size = fb->height * fb->pitch;
	struct vm_area_struct *area = get_unmapped_area(0, screen_size);
	uint32_t blocks = (area->vm_end - area->vm_start) / PMM_FRAME_SIZE;
	for (uint32_t iblock = 0; iblock < blocks; ++iblock)
		vmm_map_address(
			current_thread->parent->pdir,
			area->vm_start + iblock * PMM_FRAME_SIZE,
			fb->addr + iblock * PMM_FRAME_SIZE,
			I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER);

	elf_layout->stack -= sizeof(struct framebuffer);
	struct framebuffer *ws_fb = (struct framebuffer *)elf_layout->stack;
	memcpy(ws_fb, fb, sizeof(struct framebuffer));
	ws_fb->addr = area->vm_start;
}

void client_demo()
{
	int fd = sys_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct socket *sock = sockfd_lookup(fd);
	struct net_device *dev = get_current_net_device();

	struct sockaddr_in *sin = kcalloc(1, sizeof(struct sockaddr_in));
	sin->sin_addr = dev->local_ip;
	sin->sin_port = 10000;
	sock->ops->bind(sock, (struct sockaddr *)sin, sizeof(struct sockaddr_in));

	struct sockaddr_in *din = kcalloc(1, sizeof(struct sockaddr_in));
	// http://apache.org
	din->sin_addr = 0x284F4E01;
	din->sin_port = 80;
	while (sock->ops->connect(sock, (struct sockaddr *)din, sizeof(struct sockaddr_in)) < 0)
		;

	sock->ops->shutdown(sock);

	// 	char message[] = "GET / HTTP/1.1\r\nHost: 40.79.78.1\r\nConnection: keep-alive\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.116 Safari/537.36\r\nAccept: text/html\r\nAccept-Language: en-US\r\n\r\n";
	// 	sock->ops->sendmsg(sock, message, sizeof(message));

	// 	char *response = kcalloc(0x100000, sizeof(char));
	// 	while (sock->ops->recvmsg(sock, response, 0x100000))
	// 	{
	// 		debug_println(DEBUG_INFO, "reach ....");
	// 		memset(response, 0, 0x100000);
	// 	}
}

void kernel_init()
{
	timer_init();

	// setup random's seed
	srand(get_seconds(NULL));

	// FIXME: MQ 2019-11-19 ata_init is not called in pci_scan_buses without enabling -O2
	pci_init();
	ata_init();

	vfs_init(&ext2_fs_type, "/dev/hda");
	chrdev_memory_init();

	net_init();
	rtl8139_init();
	dhcp_setup();

	client_demo();

	// init ipc message queue
	mq_init();

	// register system apis
	syscall_init();

	// process_load("window server", "/bin/window_server", 0, setup_window_server);

	// idle
	update_thread(current_thread, THREAD_WAITING);
	schedule();

	for (;;)
		;
}

int kernel_main(unsigned long addr, unsigned long magic)
{
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
		return -1;

	struct multiboot_tag_basic_meminfo *multiboot_meminfo;
	struct multiboot_tag_mmap *multiboot_mmap;
	struct multiboot_tag_framebuffer *multiboot_framebuffer;

	struct multiboot_tag *tag;
	for (tag = (struct multiboot_tag *)(addr + 8);
		 tag->type != MULTIBOOT_TAG_TYPE_END;
		 tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7)))
	{
		switch (tag->type)
		{
		case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
			multiboot_meminfo = (struct multiboot_tag_basic_meminfo *)tag;
			break;
		case MULTIBOOT_TAG_TYPE_MMAP:
		{
			multiboot_mmap = (struct multiboot_tag_mmap *)tag;
			break;
		}
		case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
		{
			multiboot_framebuffer = (struct multiboot_tag_framebuffer *)tag;
			break;
		}
		}
	}

	// setup serial ports
	serial_init();

	// gdt including kernel, user and tss
	gdt_init();
	install_tss(5, 0x10, 0);

	// register irq and handlers
	idt_init();

	// physical memory and paging
	pmm_init(multiboot_meminfo, multiboot_mmap);
	vmm_init();

	exception_init();

	// timer and keyboard
	pit_init();
	kkybrd_install();
	mouse_init();

	framebuffer_init(multiboot_framebuffer);

	// enable interrupts to start irqs (timer, keyboard)
	enable_interrupts();

	task_init(kernel_init);

	for (;;)
		;

	return 0;
}
