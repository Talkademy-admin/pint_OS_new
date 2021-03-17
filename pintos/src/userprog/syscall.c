#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"
#include "pagedir.h"
#include "threads/palloc.h"
#include "userprog/exception.h"
#include "userprog/process.h"
#include "filesys/file-descriptor.h"

#define MAX_FD 1025

static void syscall_handler(struct intr_frame *);

/* accessing user memory */
uint32_t *assign_args(uint32_t *esp);
void *copy_user_mem_to_kernel(void *src, int size, bool null_terminated);
void *get_kernel_va_for_user_pointer(void *ptr);

/**** process control syscalls ****/
bool validate_user_pointer(void *ptr, int size);

tid_t exec(const char *file_name);

/**** file operation syscalls ****/
bool create_file_descriptor(char *buffer, struct thread *cur_thread, file_descriptor *file_d);
int is_valid_fd(long *args);
file_descriptor *get_file(int fd, struct list *fd_list);
int handle_custom_file_write(long *args, struct list *fd_list, void *buffer, unsigned size);
int handle_custom_file_read(long *args, struct list *fd_list, void *buffer, unsigned size);

static struct lock rw_lock;

/* Static list to keep global_file_descs for thread safety*/
void syscall_init(void)
{
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    lock_init(&rw_lock);
}

static void
syscall_handler(struct intr_frame *f)
{
    bool success = true;
    void *buffer = NULL;
    struct thread *cur_thread;

    // TODO now this will try to copy 16 bytes from esp to kernel memory
    // (since syscalls has at most 3 arg) but it should be dynamic based
    // on syscall cause esp can be near end and can cause killing user
    // process when everything is good
    uint32_t *args = assign_args((uint32_t *)f->esp);

    if (args == NULL)
    {
        goto kill_process;
    }

    /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

    /* printf("System call number: %d\n", args[0]); */

    switch (args[0])
    {
    /* void exit (int status) */
    case SYS_EXIT:
        f->eax = args[1];
        // set thread exit value (for wait)
        prepare_thread_for_exit(args[1]);
        thread_exit();
        break;

    /* int write (int fd, const void *buffer, unsigned size) */
    case SYS_WRITE:
        // args[2] is a pointer so we need to validate it:
        buffer = get_kernel_va_for_user_pointer((void *)args[2]);
        int w_bytes_cnt;
        if (buffer == NULL)
            goto kill_process;
        cur_thread = thread_current();
        unsigned w_size = args[3];
        switch (args[1])
        {
        case STDIN_FILENO:
            break;
        case STDOUT_FILENO:
            putbuf((char *)buffer, args[3]);
            break;
        default:
            lock_acquire(&rw_lock);
            w_bytes_cnt = handle_custom_file_write((long *)args, &cur_thread->fd_list, buffer, w_size);
            lock_release(&rw_lock);
            if (w_bytes_cnt == -1)
                success = false;
            f->eax = w_bytes_cnt;
            break;
        }
        break;

    /* int practice (int i) */
    case SYS_PRACTICE:
        f->eax = args[1] + 1;
        break;

    /* pid_t exec (const char *cmd_line) */
    case SYS_EXEC:
        buffer = get_kernel_va_for_user_pointer((void *)args[1]);
        if (buffer == NULL)
        {
            success = false;
            goto done;
        }

        tid_t child_tid = exec(buffer);
        f->eax = child_tid;
        break;

    /* int wait (pid_t pid) */
    case SYS_WAIT:
        f->eax = process_wait((tid_t)args[1]);
        break;

    case SYS_OPEN:
        buffer = get_kernel_va_for_user_pointer((void *)args[1]);
        if (buffer == NULL)
            goto kill_process;
        cur_thread = thread_current();
        file_descriptor *file_d = palloc_get_page(0);
        if (!create_file_descriptor((char *)buffer, cur_thread, file_d))
        {
            success = false;
            goto done;
        }
        f->eax = file_d->fd;
        break;

    case SYS_CLOSE:
        if ((void *)args[1] == NULL)
        {
            success = false;
            goto done;
        }
        int fd = is_valid_fd((long *)args);
        if (fd == -1)
        {
            success = false;
            goto done;
        }
        cur_thread = thread_current();
        file_descriptor *f_file = get_file(fd, &cur_thread->fd_list);
        if (f_file != NULL)
        {
            file_close(f_file->file);
            list_remove(&f_file->fd_elem);
        }
        else
            success = false;
        break;

    case SYS_CREATE:
        buffer = get_kernel_va_for_user_pointer((void *)args[1]);
        if (buffer == NULL)
        {
            success = false;
            goto kill_process;
        }

        f->eax = create_file(buffer, (unsigned)args[2]);
        break;

    case SYS_REMOVE:
        buffer = get_kernel_va_for_user_pointer((void *)args[1]);
        if (buffer == NULL)
        {
            success = false;
            goto kill_process;
        }

        f->eax = remove_file(buffer);
        break;

    case SYS_FILESIZE:
        f->eax = size_file((int)args[1]);
        break;

    case SYS_READ:;
        unsigned read_size = args[3];

        if (!validate_user_pointer((void *)args[2], read_size))
            goto kill_process;

        int read_bytes_cnt;
        cur_thread = thread_current();
        switch (args[1])
        {
        case STDIN_FILENO:
            input_getc();
            break;
        case STDOUT_FILENO:
            break;
        default:
            lock_acquire(&rw_lock);
            read_bytes_cnt = handle_custom_file_read((long *)args, &cur_thread->fd_list, args[2], read_size);
            lock_release(&rw_lock);

            f->eax = read_bytes_cnt;
            break;
        }
        break;

    case SYS_TELL:
        break;

    case SYS_SEEK:
        break;

    default:
        break;
    }

done:

    palloc_free_page(args);
    palloc_free_page(buffer);

    if (!success)
        f->eax = -1;
    return;

kill_process:
    palloc_free_page(args);
    palloc_free_page(buffer);
    prepare_thread_for_exit(-1);
    thread_exit();
}

/**** process control syscalls ****/

tid_t exec(const char *file_name)
{
    tid_t child_tid = process_execute(file_name);
    if (child_tid == TID_ERROR)
        goto done;
    struct thread *t = get_thread(child_tid);

    if (t == NULL)
    {
        child_tid = TID_ERROR;
        goto done;
    }
    if (!t->load_success_status)
    {
        child_tid = TID_ERROR;
        goto done;
    }

done:
    return child_tid;
}

/**** file operation syscalls ****/

bool create_file_descriptor(char *buffer, struct thread *cur_thread, file_descriptor *file_d)
{
    if (file_d == NULL)
        return false;
    file_d->fd = cur_thread->fd_counter++;
    struct file *o_file = filesys_open(buffer);
    if (o_file == NULL)
        return false;
    file_d->file_name = buffer;
    file_d->file = o_file;
    list_push_back(&cur_thread->fd_list, &file_d->fd_elem);
    return true;
}

int is_valid_fd(long *args)
{
    long l_fd = args[1];
    if (l_fd > MAX_FD)
        return -1;
    if (l_fd < INITIAL_FD_COUNT)
        return -1;
    return (int)(l_fd);
}

file_descriptor *get_file(int fd, struct list *fd_list)
{
    struct list_elem *e;
    file_descriptor *file_d;
    for (e = list_begin(fd_list); e != list_end(fd_list); e = list_next(e))
    {
        file_d = list_entry(e, file_descriptor, fd_elem);
        if (file_d->fd == fd)
            return file_d;
    }
    return NULL;
}

int handle_custom_file_write(long *args, struct list *fd_list, void *buffer, unsigned size)
{
    int fd = is_valid_fd(args);
    if (fd == -1)
        return -1;
    file_descriptor *f_file = get_file(fd, fd_list);
    if (f_file != NULL)
    {
        return file_write(f_file->file, buffer, size);
    }
    else
        return -1;
}

int handle_custom_file_read(long *args, struct list *fd_list, void *buffer, unsigned size)
{
    int fd = is_valid_fd(args);
    if (fd == -1)
        return -1;
    file_descriptor *f_file = get_file(fd, fd_list);
    if (f_file != NULL)
    {
        return file_read(f_file->file, buffer, size);
    }
    else
        return -1;
}

/**** accessing user memory ****/

uint32_t *
assign_args(uint32_t *esp)
{
    /* check if stack pointer is not corrupted */
    if (!is_user_vaddr(esp))
        return false;

    /* copy arguments in user stack to kernel */
    char *buffer = copy_user_mem_to_kernel(esp, MAX_SYSCALL_ARGS * 4, false);
    if (buffer == NULL)
        return false;

    return (uint32_t *)buffer;
}

/* copy user memory pointed by `ptr` to kernel memory
   and return pointer to allocated address */
void *
get_kernel_va_for_user_pointer(void *ptr)
{
    if (ptr == NULL || !is_user_vaddr(ptr))
        return NULL;

    char *buffer = copy_user_mem_to_kernel(ptr, MAX_SYSCALK_ARG_LENGTH, true);
    if (buffer == NULL)
        return NULL;

    /* set last byte in page to zero so kernel don't die in case of not valid string */
    buffer[MAX_SYSCALK_ARG_LENGTH] = 0;
    return (void *)buffer;
}

/* will check if memory from ptr to ptr+size is valid user memory */
bool 
validate_user_pointer(void *ptr, int size)
{
    if (ptr == NULL || !is_user_vaddr(ptr))
        return false;

    char *current_address = ptr;
    int seen_size = 0;
    while (seen_size < size)
    {
        if (!is_user_vaddr(current_address))
            return false;

        /* size remaining in this page */
        int cur_size = (uintptr_t)pg_round_up((void *)current_address) - (uintptr_t)current_address + 1;
        if (size - seen_size < cur_size)
            cur_size = size - seen_size;

        /* get kernel virtual address corresponding to currrent address */
        char *kernel_address = pagedir_get_page(thread_current()->pagedir, (void *)current_address);
        if (kernel_address == NULL)
            return false;

        seen_size += cur_size;
        current_address += cur_size;
    }

    return true;
}

/* copy from src in user virtual address to dst in kernel virtual
    address. if null_terminated it will continue until reaching zero,
    else it will copy `size` bytes. Max size can be PAGE_SIZE.
    (you can implement o.w. if you need.)
    will return NULL if fail. */
void *
copy_user_mem_to_kernel(void *src, int size, bool null_terminated)
{
    if (size > PGSIZE)
        return NULL;

    /* allocate a page to store data */
    char *buffer = palloc_get_page(0);
    if (buffer == NULL)
        goto fail;

    char *current_address = src;
    int copied_size = 0;
    while (copied_size < size)
    {
        if (!is_user_vaddr(current_address))
            goto fail;

        /* maximum size that we can copy in this page */
        int cur_size = (uintptr_t)pg_round_up((void *)current_address) - (uintptr_t)current_address + 1;
        if (size - copied_size < cur_size)
            cur_size = size - copied_size;

        /* get kernel virtual address corresponding to currrent address */
        char *kernel_address = pagedir_get_page(thread_current()->pagedir, (void *)current_address);
        if (kernel_address == NULL)
            goto fail;

        memcpy(buffer + copied_size, kernel_address, cur_size);

        /* we will search for null here if we need to copy until null */
        if (null_terminated)
        {
            char *temp = buffer + copied_size;
            bool null_found = false;
            while (temp < buffer + copied_size + cur_size)
            {
                if (!(*temp))
                    null_found = true;
                temp++;
            }
            // if null found stop copy.
            if (null_found)
                break;
        }

        copied_size += cur_size;
        current_address += cur_size;
    }

    return buffer;

fail:
    if (buffer)
        palloc_free_page(buffer);
    return NULL;
}
