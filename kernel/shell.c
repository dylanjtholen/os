#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

int serial_init(void);
void serial_putc(char c);
int serial_getc(void);
void serial_write(const char *s);

static void putstr(const char *s) { serial_write(s); }
static void putch(char c) { serial_putc(c); }

enum
{
    KEY_NONE = 0,
    KEY_CHAR, /* any normal char */
    KEY_BACKSPACE,
    KEY_ENTER,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_DELETE,
    KEY_INSERT,
};

int read_key(int *out_char)
{
    int c = serial_getc();
    if (c == '\r' || c == '\n')
    {
        return KEY_ENTER;
    }
    if (c == 0x7f || c == 8)
    { /* delete or backspace */
        return KEY_BACKSPACE;
    }
    if (c != 0x1B)
    {
        *out_char = c;
        return KEY_CHAR;
    }

    // escape sequence
    int c2 = serial_getc();
    if (c2 == '[')
    {
        int c3 = serial_getc();
        if (c3 >= '0' && c3 <= '9')
        {

            int num = c3 - '0';
            int nc;
            while ((nc = serial_getc()) >= 0 && nc >= '0' && nc <= '9')
            {
                num = num * 10 + (nc - '0');
            }
            /* nc is the terminator, usually '~' */
            if (nc == '~')
            {
                switch (num)
                {
                case 1:
                    return KEY_HOME;
                case 2:
                    return KEY_INSERT;
                case 3:
                    return KEY_DELETE;
                case 4:
                    return KEY_END;
                default:
                    return KEY_NONE;
                }
            }
            else
            {
                /* unexpected terminator, fall back */
                return KEY_NONE;
            }
        }
        else
        {
            /* common short sequences */
            switch (c3)
            {
            case 'A':
                return KEY_UP;
            case 'B':
                return KEY_DOWN;
            case 'C':
                return KEY_RIGHT;
            case 'D':
                return KEY_LEFT;
            case 'H':
                return KEY_HOME;
            case 'F':
                return KEY_END;
            default:
                return KEY_NONE;
            }
        }
    }
    else if (c2 == 'O')
    {
        int c3 = serial_getc();
        switch (c3)
        {
        case 'A':
            return KEY_UP;
        case 'B':
            return KEY_DOWN;
        case 'C':
            return KEY_RIGHT;
        case 'D':
            return KEY_LEFT;
        case 'H':
            return KEY_HOME;
        case 'F':
            return KEY_END;
        default:
            return KEY_NONE;
        }
    }
    else
    {
        /* literal escape */
        return KEY_NONE;
    }
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b)
    {
        ++a;
        ++b;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int str_len(const char *s)
{
    int len = 0;
    while (s[len])
        ++len;
    return len;
}

// commands
static void cmd_help(int argc, char **argv)
{
    putstr("help is the only command rn lmao\n");
}

struct cmd
{
    const char *name;
    void (*fn)(int argc, char **argv);
};

static struct cmd cmds[] = {
    {"help", cmd_help},
};

#define MAXLINE 128
static char linebuf[MAXLINE];

static const char *prompt = "> ";
static const int prompt_len = 2;

/* redraw the entire input line and place the cursor at `pos` */
static void redraw_line(int pos)
{
    int len = str_len(linebuf);
    /* move to start of line and print prompt + buffer */
    serial_write("\r");
    serial_write(prompt);
    serial_write(linebuf);
    /* move cursor back from end to desired pos */
    int back = len - pos;
    for (int i = 0; i < back; ++i)
        putch('\b');
}

static int tokenize(char *line, char *argv[])
{
    int argc = 0;
    char *p = line;
    while (*p && argc < 32)
    {
        while (*p == ' ' || *p == '\t')
            ++p;
        if (*p == '\0')
            break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t')
            ++p;
        if (*p)
        {
            *p = '\0';
            ++p;
        }
    }
    return argc;
}

void shell_main()
{
    if (serial_init() != 0)
    {
        putstr("Failed to initialize serial port\n");
        asm("hlt");
    }
    putstr("Welcome to [OS Name]!\n");
    putstr("For a list of commands, type 'help'\n");
    int insert_mode = 1; // 0 for overwrite, 1 for insert
    for (;;)
    {
        serial_write(prompt);
        int pos = 0;
        linebuf[0] = '\0';
        while (1)
        {
            int c;
            int type = read_key(&c);
            if (type == KEY_ENTER)
            {
                putstr("\r\n");
                break;
            }
            switch (type)
            {
            case KEY_NONE:
                continue;
            case KEY_BACKSPACE:
                if (pos > 0)
                {
                    int len = str_len(linebuf);
                    /* shift left from pos-1 */
                    for (int i = pos - 1; i < len; ++i)
                        linebuf[i] = linebuf[i + 1];
                    pos--;
                    redraw_line(pos);
                }
                break;
            case KEY_CHAR:
                if (insert_mode)
                {
                    int len = str_len(linebuf);
                    if (len < MAXLINE - 1 && pos <= len)
                    {
                        /* shift right */
                        for (int i = len; i >= pos; --i)
                            linebuf[i + 1] = linebuf[i];
                        linebuf[pos] = (char)c;
                        pos++;
                        redraw_line(pos);
                    }
                }
                else
                {
                    int len = str_len(linebuf);
                    if (pos < MAXLINE - 1 && pos <= len)
                    {
                        linebuf[pos] = (char)c;
                        if (pos == len)
                        {
                            linebuf[pos + 1] = '\0';
                        }
                        pos++;
                        redraw_line(pos);
                    }
                }
                break;
            case KEY_INSERT:
                insert_mode = !insert_mode;
                break;
            case KEY_LEFT:
                if (pos > 0)
                {
                    pos--;
                    putch('\b');
                }
                break;
            case KEY_RIGHT:
            {
                int len = str_len(linebuf);
                if (pos < len)
                {
                    putch(linebuf[pos]);
                    pos++;
                }
            }
            break;
            }
        }
        linebuf[pos] = '\0';
        char *argv[32];
        int argc = tokenize(linebuf, argv);
        if (argc == 0)
            continue;
        bool handled = false;
        for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); ++i)
        {
            if (!strcmp(argv[0], cmds[i].name))
            {
                cmds[i].fn(argc, argv);
                handled = true;
                break;
            }
        }
        if (!handled)
        {
            putstr("unknown command\n");
        }
    }
}