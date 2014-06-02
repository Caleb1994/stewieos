#include "kernel.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"
#include "kmem.h"
#include "fs.h"
#include "task.h"
#include "multiboot.h"
#include <sys/fcntl.h>
#include "sys/mount.h"
#include "elf/elf32.h"
#include "error.h"
#include "sys/stat.h"
#include "sys/types.h"
#include <unistd.h>

// testing spinlock
#include "spinlock.h"

tick_t my_timer_callback(tick_t, struct regs*);
tick_t my_timer_callback(tick_t time, struct regs* regs)
{
	UNUSED(regs);
	printk("timer_callback: time: %d+%d/1000\n", (time / 1000), time%1000);
	return time+timer_get_freq();
}

// defined in initial_printk.c
//void clear_screen(void);

int global_var = 0;

void multitasking_entry( multiboot_info_t* mb);
int initfs_install(multiboot_info_t* mb);

int kmain( multiboot_info_t* mb );
int kmain( multiboot_info_t* mb )
{
	//clear_screen();
	initialize_descriptor_tables();
	asm volatile("sti");
	init_timer(1000); // init the timer at 1000hz
	printk("initializing paging... ");
	unsigned int curpos = get_cursor_pos();
	printk("\n");
	init_paging(mb); // initialize the page tables and enable paging
	printk_at(curpos, " done.\n");

	char cpu_vendor[16] = {0};
	u32 max_code = cpuid_string(CPUID_GETVENDORSTRING, cpu_vendor);
	
	printk("CPU Vendor String: %s (maximum supported cpuid code: %d)\n", &cpu_vendor[0], max_code);
	
	
	/*printk("Registering timer callback for the next second... ");
	int result = timer_callback(timer_get_ticks()+timer_get_freq(), my_timer_callback);
	printk("done (result=%d)\n", result);*/
	
	printk("Initializing virtual filesystem... ");
	initialize_filesystem();
	printk("done.\n");
	
	printk("Initializing multitasking subsystem...\n");
	task_init();
	
	printk("init: forking init task... ");
	
	pid_t pid = sys_fork();
	
	if( pid == 0 ){
		multitasking_entry(mb);
		// this should never happen
		sys_exit(-1);
	}
	
	while(1);
	
	return (int)0xdeadbeaf;
}

void multitasking_entry( multiboot_info_t* mb )
{
	//int status = 0;
	//pid_t pid = 0;
	//const char* filename = "/somefile.txt";
	int fd = 0;
	
	printk("done.\n");
	//while(1);
	
	initfs_install(mb);
	
	printk("INIT: Opening a module... ");
	fd = sys_open("/test_mod.o", O_RDONLY, 0);
	printk(" done (result: %d)\n", fd);
	if( fd < 0 ){
		printk("INIT: Unable to open module.\n");
	} else {
		struct stat file_info;
		sys_fstat(fd, &file_info);
		char* file_data = kmalloc(file_info.st_size);
		sys_lseek(fd, 0, SEEK_SET);
		sys_read(fd, file_data, file_info.st_size);
		module_t* module = elf_init_module(file_data, file_info.st_size);
		if( IS_ERR(module) ){
			printk("INIT: error: unable to load module (error: %d).\n", PTR_ERR(module));
			kfree(file_data);
		} else {
			printk("INIT: module load result code: %d\n", module->m_load(module));
			printk("INIT: module remove result code: %d\n", module->m_remove(module));
			kfree(module->m_loadaddr);
		}
		sys_close(fd);
	}
	/*
	printk("INIT: mounting testfs to \"/\"...\n");
	int result = sys_mount("", "/", "testfs", MS_RDONLY, NULL);
	printk("INIT: mounting result: %i\n", result);
	
	printk("INIT: attempting to open file %s\n", filename);

	fd = sys_open(filename, O_RDONLY, 0);
	
	
	if( fd >= 0 ){
		printk("INIT: successfully opened %s: file descriptor %i\n", filename, fd);
	} else {
		printk("INIT: unable to open %s: error code %i\n", filename, fd);
	}
	
	printk("INIT: attempting to unmount while file is open...\n");
	result = sys_umount("/", 0);
	if( result == 0 ){
		printk("INIT: Uh-Oh, I was able to unmount with an open file...\n");
	} else {
		printk("INIT: Good, that didn't go well (error %d).\n", result);
	}
	
	printk("INIT: closing open file descriptor...\n");
	sys_close(fd);
	
	printk("INIT: attempting to unmount the filesystem again...\n");
	result = sys_umount("/", 0);
	if( result == 0 ){
		printk("INIT: the filesystem was successfully unmounted!\n");
	} else {
		printk("INIT: filesystem could not be unmounted (error %d)\n", result);
	}
	
	printk("INIT: attempting to open a file from old mount...\n");
	fd = sys_open(filename, O_RDONLY, 0);
	if( fd >= 0 ){
		printk("INIT: successfully opened %s: file descriptor: %i\n", filename, fd);
	} else {
		printk("INIT: unable to open %s (error %d).\n", filename, fd);
	}*/
	
	printk("INIT: Finished.\n");
	
	return;
	printk("INIT: creating a new spinlock.\n");
	spinlock_t* lock = (spinlock_t*)kmalloc(sizeof(spinlock_t));
	printk("INIT: initializing the lock to `unlocked' state.\n");
	spin_init(lock);
	printk("INIT: attempting a fork.\n");
	sys_fork();
	printk("INIT(%d): forked init process.\n", sys_getpid());
	
	spin_lock(lock);
	printk("INIT(%d): Process %d locked the spinlock.\n", sys_getpid(), sys_getpid());
	tick_t wait_end = timer_get_ticks()+5*timer_get_freq();
	while( wait_end > timer_get_ticks() );
	printk("INIT(%d): Process %d unlocking...\n", sys_getpid(), sys_getpid());
	spin_unlock(lock);
	
}