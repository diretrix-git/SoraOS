#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "pmm.h"
#include "heap.h"
#include "timer.h"
#include "process.h"
#include "scheduler.h"

#define SHELL_BUF 256

static char cmd_buf[SHELL_BUF];
static int cmd_len = 0;

/* ------------------------------------------------------------------ */
/* Internal string helpers                                             */
/* ------------------------------------------------------------------ */
static int sh_strcmp(const char *a, const char *b)
{
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
}

static int sh_strlen(const char *s)
{
    int n = 0;
    while (s[n])
        n++;
    return n;
}

/* ------------------------------------------------------------------ */
/* Welcome banner                                                      */
/* ------------------------------------------------------------------ */
static void print_banner(void)
{
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("\n");
    vga_print("  +------------------------------------------+\n");
    vga_print("  |         MyOS v1.1  —  Hobby OS           |\n");
    vga_print("  |   Preemptive multitasking / PS/2 kbd     |\n");
    vga_print("  +------------------------------------------+\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("  Type 'help' for a list of commands.\n\n");
}

/* ------------------------------------------------------------------ */
/* Prompt                                                              */
/* ------------------------------------------------------------------ */
static void print_prompt(void)
{
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("myos");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_print("> ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

/* ------------------------------------------------------------------ */
/* Read one line from keyboard into cmd_buf                           */
/* ------------------------------------------------------------------ */
static void read_line(void)
{
    cmd_len = 0;
    while (1)
    {
        char c = keyboard_getchar();

        if (c == '\n' || c == '\r')
        {
            vga_putchar('\n');
            cmd_buf[cmd_len] = '\0';
            return;
        }

        if (c == '\b')
        {
            if (cmd_len > 0)
            {
                cmd_len--;
                vga_putchar('\b');
                vga_putchar(' ');
                vga_putchar('\b');
            }
            continue;
        }

        /* Printable ASCII */
        if (c >= 0x20 && c < 0x7F)
        {
            if (cmd_len < SHELL_BUF - 1)
            {
                cmd_buf[cmd_len++] = c;
                vga_putchar(c);
            }
        }
    }
}

/* ================================================================== */
/* Command implementations                                             */
/* ================================================================== */

static void cmd_help(void)
{
    vga_print("Available commands:\n");
    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("  help     ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("- Show this message\n");

    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("  clear    ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("- Clear the screen\n");

    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("  echo     ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("- Echo text back  (echo <text>)\n");

    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("  meminfo  ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("- Physical memory (PMM) statistics\n");

    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("  heapinfo ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("- Kernel heap statistics\n");

    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("  ps       ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("- Show running process\n");

    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("  timer    ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("- Show timer tick counter\n");

    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("  yield    ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("- Voluntarily yield CPU to other processes\n");

    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("  reboot   ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("- Reboot via triple fault\n");
}

/* ------------------------------------------------------------------ */
static void cmd_meminfo(void)
{
    uint32_t used_f = pmm_used_frames();
    uint32_t free_f = pmm_free_frames();

    vga_print("Physical Memory (PMM):\n");
    vga_print("  Frame size : 4 KB\n");
    vga_print("  Used frames: ");
    vga_print_int((int32_t)used_f);
    vga_print("  (");
    vga_print_int((int32_t)(used_f * 4));
    vga_print(" KB)\n");
    vga_print("  Free frames: ");
    vga_print_int((int32_t)free_f);
    vga_print("  (");
    vga_print_int((int32_t)(free_f * 4));
    vga_print(" KB)\n");
    vga_print("  Total      : ");
    vga_print_int((int32_t)((used_f + free_f) * 4));
    vga_print(" KB\n");
}

/* ------------------------------------------------------------------ */
static void cmd_heapinfo(void)
{
    uint32_t used = heap_used();
    uint32_t free = heap_free();

    vga_print("Kernel Heap (kmalloc):\n");
    vga_print("  Heap start : 0x200000\n");
    vga_print("  Used       : ");
    vga_print_int((int32_t)used);
    vga_print(" bytes\n");
    vga_print("  Free       : ");
    vga_print_int((int32_t)free);
    vga_print(" bytes\n");
    vga_print("  Total      : ");
    vga_print_int((int32_t)(used + free));
    vga_print(" bytes\n");
}

/* ------------------------------------------------------------------ */
static void cmd_ps(void)
{
    struct process *p = scheduler_get_current();
    vga_print("PID   Name              State\n");
    vga_print("----  ----------------  -------\n");
    if (p)
    {
        vga_print_int((int32_t)p->pid);
        vga_print("     ");
        vga_print(p->name);
        /* pad name */
        int pad = 18 - sh_strlen(p->name);
        while (pad-- > 0)
            vga_putchar(' ');
        vga_print("RUNNING\n");
    }
    else
    {
        vga_print("  (no current process)\n");
    }
}

/* ------------------------------------------------------------------ */
static void cmd_timer(void)
{
    vga_print("Timer ticks : ");
    vga_print_int((int32_t)timer_get_ticks());
    vga_print("  (");
    vga_print_int((int32_t)(timer_get_ticks() / 100));
    vga_print(" s at 100 Hz)\n");
}

/* ------------------------------------------------------------------ */
static void cmd_echo(const char *args)
{
    if (!args || !*args)
    {
        vga_putchar('\n');
        return;
    }
    vga_print(args);
    vga_putchar('\n');
}

/* ------------------------------------------------------------------ */
static void cmd_reboot(void)
{
    vga_set_color(VGA_WHITE, VGA_RED);
    vga_print("\nRebooting...\n");

    /* Triple-fault by loading a null IDT and triggering a software int */
    struct
    {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) null_idtr = {0, 0};

    __asm__ volatile(
        "cli           \n"
        "lidt %0       \n"
        "int  $0       \n"
        : : "m"(null_idtr));

    while (1)
        __asm__ volatile("hlt");
}

/* ================================================================== */
/* Command dispatcher                                                  */
/* ================================================================== */
static void execute(char *line)
{
    /* Skip leading whitespace */
    while (*line == ' ')
        line++;
    if (!*line)
        return;

    /* Split into command and argument string */
    char *cmd = line;
    char *args = 0;
    char *p = line;
    while (*p && *p != ' ')
        p++;
    if (*p == ' ')
    {
        *p = '\0';
        args = p + 1;
        while (*args == ' ')
            args++; /* skip leading spaces in args */
    }

    if (sh_strcmp(cmd, "help") == 0)
        cmd_help();
    else if (sh_strcmp(cmd, "clear") == 0)
        vga_clear();
    else if (sh_strcmp(cmd, "echo") == 0)
        cmd_echo(args);
    else if (sh_strcmp(cmd, "meminfo") == 0)
        cmd_meminfo();
    else if (sh_strcmp(cmd, "heapinfo") == 0)
        cmd_heapinfo();
    else if (sh_strcmp(cmd, "ps") == 0)
        cmd_ps();
    else if (sh_strcmp(cmd, "timer") == 0)
        cmd_timer();
    else if (sh_strcmp(cmd, "yield") == 0)
        scheduler_yield();
    else if (sh_strcmp(cmd, "reboot") == 0)
        cmd_reboot();
    else
    {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_print("Unknown command: ");
        vga_print(cmd);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_print("  (try 'help')\n");
    }
}

/* ================================================================== */
/* Public API                                                          */
/* ================================================================== */
void shell_init(void)
{
    print_banner();
}

void shell_run(void)
{
    while (1)
    {
        print_prompt();
        read_line();
        execute(cmd_buf);
    }
}