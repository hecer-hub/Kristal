#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>

#define MAX_VARS 100
#define MAX_VAR_NAME 50
#define MAX_VAR_VALUE 256
#define MAX_PATH 512

int error_to_file = 0;
FILE* error_file = NULL;

void process_stv(char* line);
void process_ourip(char* line);
void process_rd(char* line);
void process_ifi(char* line, FILE* file);
void process_arc(char* line);
void process_unarc(char* line);
void process_wf(char* line);
void process_rf(char* line);
void process_lvl(char* line);
void process_clear(char* line);
void process_math(char* line);
void process_grep(char* line);
void process_cat(char* line);
void process_help(void);
void process_kristal_file(const char* filename);
void archive(const char* output_file, const char* input_files[], const char* levels[], int file_count);
void unarchive(const char* input_file, const char* output_dir);
void add_directory_to_archive(const char* dir_name, const char* output_file, const char* level);
void log_error(const char* format, ...);

struct Variable {
    char name[MAX_VAR_NAME];
    char value[MAX_VAR_VALUE];
};

struct Variable vars[MAX_VARS];
int var_count = 0;
int is_kernel = 0;

const char* get_var(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            return vars[i].value;
        }
    }
    return NULL;
}

void set_var(const char* name, const char* value) {
    if (var_count >= MAX_VARS) {
        log_error("Ошибка: превышен лимит переменных\n");
        return;
    }
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            strncpy(vars[i].value, value, MAX_VAR_VALUE);
            printf("Переменная %s обновлена: %s\n", name, value);
            return;
        }
    }
    strncpy(vars[var_count].name, name, MAX_VAR_NAME);
    strncpy(vars[var_count].value, value, MAX_VAR_VALUE);
    printf("Переменная %s установлена: %s\n", name, value);
    var_count++;
}

void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    if (error_to_file) {
        if (!error_file) error_file = fopen("error.txt", "a");
        if (error_file) {
            vfprintf(error_file, format, args);
            fflush(error_file);
        }
    } else {
        vfprintf(stderr, format, args);
    }
    va_end(args);
}

void process_stv(char* line) {
    char* token = strtok(line, " =");
    token = strtok(NULL, " =\"");
    if (!token) {
        log_error("Ошибка: неверный формат команды stv\n");
        return;
    }
    char* name = token;
    token = strtok(NULL, "\"");
    if (!token) {
        log_error("Ошибка: отсутствует значение для stv\n");
        return;
    }
    set_var(name, token);
}

void process_ourip(char* line) {
    char* token = strtok(line, "['");
    token = strtok(NULL, "' ");
    if (!token) {
        log_error("Ошибка: неверный формат команды ourip\n");
        return;
    }
    char text[256];
    strncpy(text, token, 255);
    text[255] = '\0';
    text[strcspn(text, "'")] = '\0';
    token = strtok(NULL, "' ]");
    if (token && token[0] == '@') {
        token++;
        const char* var_value = get_var(token);
        if (!var_value) {
            log_error("Ошибка: переменная %s не найдена\n", token);
            return;
        }
        printf("%s %s\n", text, var_value);
    } else {
        printf("%s\n", text);
    }
}

void process_rd(char* line) {
    char* token = strtok(line, " ");
    token = strtok(NULL, " ['");
    if (!token) {
        log_error("Ошибка: неверный формат команды rd\n");
        return;
    }
    char* var_name = token;
    token = strtok(NULL, "' ]");
    if (token) {
        char prompt[256];
        strncpy(prompt, token, 255);
        prompt[255] = '\0';
        prompt[strcspn(prompt, "'")] = '\0';
        printf("%s", prompt);
        fflush(stdout);
    }
    char input[256];
    if (fgets(input, 256, stdin)) {
        input[strcspn(input, "\n")] = 0;
        set_var(var_name, input);
    } else {
        log_error("Ошибка чтения ввода\n");
    }
}

void process_ifi(char* line, FILE* file) {
    char* token = strtok(line, " =");
    token = strtok(NULL, " =");
    if (!token) {
        log_error("Ошибка: неверный формат команды ifi\n");
        return;
    }
    char* var_name = token;
    const char* var_value = get_var(var_name);
    if (!var_value) {
        log_error("Ошибка: переменная %s не найдена\n", var_name);
        return;
    }
    token = strtok(NULL, " ");
    if (!token) {
        log_error("Ошибка: отсутствует значение для сравнения\n");
        return;
    }
    char* compare_value = token;
    char if_block[1024] = {0};
    char else_block[1024] = {0};
    char current_line[256];
    int in_if_block = 0, in_else_block = 0;
    while (fgets(current_line, 256, file)) {
        current_line[strcspn(current_line, "\n")] = 0;
        if (strstr(current_line, "[")) in_if_block = 1;
        else if (strstr(current_line, "j [")) in_if_block = 0, in_else_block = 1;
        else if (strstr(current_line, "]")) {
            if (in_if_block) in_if_block = 0;
            else if (in_else_block) in_else_block = 0;
            break;
        } else if (in_if_block) {
            strncat(if_block, current_line, 1024 - strlen(if_block) - 1);
            strncat(if_block, "\n", 1024 - strlen(if_block) - 1);
        } else if (in_else_block) {
            strncat(else_block, current_line, 1024 - strlen(else_block) - 1);
            strncat(else_block, "\n", 1024 - strlen(else_block) - 1);
        }
    }
    char* block = (strcmp(var_value, compare_value) == 0) ? if_block : else_block;
    char* line_copy = strdup(block);
    char* cmd = strtok(line_copy, "\n");
    while (cmd) {
        if (strncmp(cmd, "ourip", 5) == 0) process_ourip(cmd);
        else if (strncmp(cmd, "rd", 2) == 0) process_rd(cmd);
        else if (strncmp(cmd, "stv", 3) == 0) process_stv(cmd);
        else if (strncmp(cmd, "arc", 3) == 0) process_arc(cmd);
        else if (strncmp(cmd, "unarc", 5) == 0) process_unarc(cmd);
        else if (strncmp(cmd, "wf", 2) == 0) process_wf(cmd);
        else if (strncmp(cmd, "rf", 2) == 0) process_rf(cmd);
        else if (strncmp(cmd, "lvl", 3) == 0) process_lvl(cmd);
        else if (strncmp(cmd, "clear", 5) == 0) process_clear(cmd);
        else if (strncmp(cmd, "math", 4) == 0) process_math(cmd);
        else if (strncmp(cmd, "grep", 4) == 0) process_grep(cmd);
        else if (strncmp(cmd, "cat", 3) == 0) process_cat(cmd);
        else if (strncmp(cmd, "help", 4) == 0) process_help();
        cmd = strtok(NULL, "\n");
    }
    free(line_copy);
}

void add_directory_to_archive(const char* dir_name, const char* output_file, const char* level) {
    DIR* dir = opendir(dir_name);
    if (!dir) {
        log_error("Ошибка открытия директории %s!\n", dir_name);
        return;
    }

    struct dirent* entry;
    char path[MAX_PATH];
    const char* input_files[100];
    const char* levels[100];
    int file_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        snprintf(path, MAX_PATH, "%s/%s", dir_name, entry->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            input_files[file_count] = strdup(path);
            levels[file_count] = level;
            file_count++;
        }
    }
    closedir(dir);

    if (file_count > 0) {
        archive(output_file, input_files, levels, file_count);
    }

    for (int i = 0; i < file_count; i++) {
        free((void*)input_files[i]);
    }
}

void archive(const char* output_file, const char* input_files[], const char* levels[], int file_count) {
    FILE* archive = fopen(output_file, "wb");
    if (archive == NULL) {
        log_error("Ошибка открытия архива %s!\n", output_file);
        return;
    }
    char buffer[1024];
    for (int i = 0; i < file_count; i++) {
        FILE* input = fopen(input_files[i], "rb");
        if (input == NULL) {
            log_error("Ошибка открытия файла %s!\n", input_files[i]);
            continue;
        }

        size_t level_len = strlen(levels[i]);
        fwrite(&level_len, sizeof(size_t), 1, archive);
        fwrite(levels[i], sizeof(char), level_len, archive);

        size_t name_len = strlen(input_files[i]);
        fwrite(&name_len, sizeof(size_t), 1, archive);
        fwrite(input_files[i], sizeof(char), name_len, archive);

        size_t bytes_read, total_bytes = 0;
        while ((bytes_read = fread(buffer, 1, 1024, input)) > 0) {
            total_bytes += bytes_read;
        }
        fseek(input, 0, SEEK_SET);
        fwrite(&total_bytes, sizeof(size_t), 1, archive);
        while ((bytes_read = fread(buffer, 1, 1024, input)) > 0) {
            fwrite(buffer, 1, bytes_read, archive);
        }
        fclose(input);
        printf("Архивирован файл: %s (уровень %s)\n", input_files[i], levels[i]);
    }
    fclose(archive);
    printf("Архив создан: %s\n", output_file);
}

void process_arc(char* line) {
    char* token = strtok(line, "[");
    if (!token) {
        log_error("Ошибка: некорректный формат команды arc\n");
        return;
    }
    token = strtok(NULL, "'");
    if (!token) {
        log_error("Ошибка: отсутствует имя архива\n");
        return;
    }
    char archive_file[256];
    strncpy(archive_file, token, 255);
    archive_file[255] = '\0';
    archive_file[strcspn(archive_file, "'")] = '\0';

    const char* input_files[10];
    const char* levels[10];
    int count = 0;

    char* rest = strtok(NULL, "]");
    if (!rest) {
        log_error("Ошибка: отсутствуют файлы для архивации\n");
        return;
    }
    char* file_token = strtok(rest, "' ");
    while (file_token && count < 10) {
        if (file_token[0] == '\0' || file_token[0] == ']') break;
        while (isspace(*file_token)) file_token++;
        if (strlen(file_token) == 0) {
            file_token = strtok(NULL, "' ");
            continue;
        }
        struct stat st;
        if (stat(file_token, &st) == 0 && S_ISDIR(st.st_mode)) {
            add_directory_to_archive(file_token, archive_file, "p");
        } else {
            input_files[count] = file_token;
            levels[count] = "p";
            count++;
        }
        file_token = strtok(NULL, "' ");
    }

    if (count == 0 && !strstr(rest, "/")) {
        log_error("Ошибка: не указаны файлы для архивации\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        FILE* test = fopen(input_files[i], "r");
        if (!test) {
            log_error("Предупреждение: файл %s не существует\n", input_files[i]);
        } else {
            fclose(test);
        }
    }

    if (count > 0) {
        archive(archive_file, input_files, levels, count);
    }
}

void unarchive(const char* input_file, const char* output_dir) {
    FILE* archive = fopen(input_file, "rb");
    if (archive == NULL) {
        log_error("Ошибка открытия архива %s!\n", input_file);
        return;
    }

    mkdir(output_dir, 0777);
    char buffer[1024];
    while (!feof(archive)) {
        size_t level_len;
        if (fread(&level_len, sizeof(size_t), 1, archive) != 1) break;
        char level[256];
        fread(level, sizeof(char), level_len, archive);
        level[level_len] = '\0';

        size_t name_len;
        fread(&name_len, sizeof(size_t), 1, archive);
        char name[256];
        fread(name, sizeof(char), name_len, archive);
        name[name_len] = '\0';

        char output_path[512];
        snprintf(output_path, 512, "%s/%s", output_dir, name);

        char* dir_path = strdup(output_path);
        char* last_slash = strrchr(dir_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            mkdir(dir_path, 0777);
        }
        free(dir_path);

        size_t file_size;
        fread(&file_size, sizeof(size_t), 1, archive);

        FILE* output = fopen(output_path, "wb");
        if (output == NULL) {
            log_error("Ошибка создания файла %s!\n", output_path);
            continue;
        }

        size_t total_read = 0;
        while (total_read < file_size) {
            size_t to_read = file_size - total_read > 1024 ? 1024 : file_size - total_read;
            size_t bytes_read = fread(buffer, 1, to_read, archive);
            fwrite(buffer, 1, bytes_read, output);
            total_read += bytes_read;
        }
        fclose(output);
        printf("Распакован файл: %s (уровень %s)\n", output_path, level);
    }
    fclose(archive);
    printf("Распаковка завершена в папке: %s\n", output_dir);
}

void process_unarc(char* line) {
    char* token = strtok(line, "['");
    token = strtok(NULL, "' ");
    if (!token) {
        log_error("Ошибка: неверный формат команды unarc\n");
        return;
    }
    char* archive_file = token;
    token = strtok(NULL, "' ]");
    if (!token) {
        log_error("Ошибка: отсутствует директория для unarc\n");
        return;
    }
    char* output_dir = token;
    if (output_dir[0] == '@') {
        output_dir++;
        const char* var_value = get_var(output_dir);
        if (!var_value) {
            log_error("Ошибка: переменная %s не найдена\n", output_dir);
            return;
        }
        output_dir = (char*)var_value;
    }
    unarchive(archive_file, output_dir);
}

void process_wf(char* line) {
    char* token = strtok(line, "['");
    token = strtok(NULL, "' ");
    if (!token) {
        log_error("Ошибка: неверный формат команды wf\n");
        return;
    }
    char* filename = token;
    token = strtok(NULL, "' ]");
    if (!token) {
        log_error("Ошибка: отсутствуют данные для wf\n");
        return;
    }
    char* data = token;
    if (data[0] == '@') {
        data++;
        const char* var_value = get_var(data);
        if (!var_value) {
            log_error("Ошибка: переменная %s не найдена\n", data);
            return;
        }
        data = (char*)var_value;
    }
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        log_error("Ошибка открытия файла %s!\n", filename);
        return;
    }
    fputs(data, file);
    fclose(file);
    printf("Записано в файл %s: %s\n", filename, data);
}

void process_rf(char* line) {
    char* token = strtok(line, "['");
    token = strtok(NULL, "' ]");
    if (!token) {
        log_error("Ошибка: неверный формат команды rf\n");
        return;
    }
    FILE* file = fopen(token, "r");
    if (file == NULL) {
        log_error("Ошибка открытия файла %s!\n", token);
        return;
    }
    char buffer[1024];
    while (fgets(buffer, 1024, file)) {
        printf("Прочитано из %s: %s\n", token, buffer);
    }
    fclose(file);
}

void process_lvl(char* line) {
    char* token = strtok(line, "['");
    token = strtok(NULL, "' ");
    if (!token) {
        log_error("Ошибка: неверный формат команды lvl\n");
        return;
    }
    char* filename = token;
    token = strtok(NULL, "' ]");
    if (!token) {
        log_error("Ошибка: отсутствует уровень доступа\n");
        return;
    }
    if (strcmp(token, "p") != 0 && strcmp(token, "u") != 0 && strcmp(token, "k") != 0) {
        log_error("Ошибка: неверный уровень доступа %s\n", token);
        return;
    }
    printf("Установлен уровень доступа для %s: %s\n", filename, token);
}

void process_clear(char* line) {
    system("clear");
}

void process_math(char* line) {
    char* token = strtok(line, " ");
    token = strtok(NULL, " =");
    if (!token) {
        log_error("Ошибка: неверный формат команды math\n");
        return;
    }
    char* var_name = token;
    token = strtok(NULL, " ");
    if (!token) {
        log_error("Ошибка: отсутствует первое значение\n");
        return;
    }
    double a;
    if (token[0] == '@') {
        const char* var_value = get_var(token + 1);
        if (!var_value) {
            log_error("Ошибка: переменная %s не найдена\n", token + 1);
            return;
        }
        a = atof(var_value);
    } else {
        a = atof(token);
    }
    token = strtok(NULL, " ");
    if (!token) {
        log_error("Ошибка: отсутствует оператор\n");
        return;
    }
    char* op = token;
    token = strtok(NULL, " ");
    if (!token) {
        log_error("Ошибка: отсутствует второе значение\n");
        return;
    }
    double b;
    if (token[0] == '@') {
        const char* var_value = get_var(token + 1);
        if (!var_value) {
            log_error("Ошибка: переменная %s не найдена\n", token + 1);
            return;
        }
        b = atof(var_value);
    } else {
        b = atof(token);
    }
    double result;
    if (strcmp(op, "+") == 0) result = a + b;
    else if (strcmp(op, "-") == 0) result = a - b;
    else if (strcmp(op, "*") == 0) result = a * b;
    else if (strcmp(op, "/") == 0) {
        if (b == 0) {
            log_error("Ошибка: деление на ноль\n");
            return;
        }
        result = a / b;
    } else {
        log_error("Ошибка: неизвестный оператор %s\n", op);
        return;
    }
    char result_str[256];
    snprintf(result_str, 256, "%g", result);
    set_var(var_name, result_str);
}

void process_grep(char* line) {
    char* token = strtok(line, "['");
    token = strtok(NULL, "' ");
    if (!token) {
        log_error("Ошибка: неверный формат команды grep\n");
        return;
    }
    char* pattern = token;
    token = strtok(NULL, "' ]");
    if (!token) {
        log_error("Ошибка: отсутствует файл для grep\n");
        return;
    }
    char* filename = token;
    FILE* file = fopen(filename, "r");
    if (!file) {
        log_error("Ошибка открытия файла %s!\n", filename);
        return;
    }
    char buffer[1024];
    int line_num = 0;
    while (fgets(buffer, 1024, file)) {
        line_num++;
        if (strstr(buffer, pattern)) {
            printf("%s:%d:%s", filename, line_num, buffer);
        }
    }
    fclose(file);
}

void process_cat(char* line) {
    char* token = strtok(line, "['");
    token = strtok(NULL, "' ]");
    if (!token) {
        log_error("Ошибка: неверный формат команды cat\n");
        return;
    }
    FILE* file = fopen(token, "r");
    if (!file) {
        log_error("Ошибка открытия файла %s!\n", token);
        return;
    }
    char buffer[1024];
    while (fgets(buffer, 1024, file)) {
        printf("%s", buffer);
    }
    fclose(file);
}

void process_help() {
    printf(" __  __  ______  _______  _______  _______  _______  _____\n");
    printf("|  |/  ||   __ \\|_     _||     __||_     _||   _   ||     |_\n");
    printf("|     < |      < _|   |_ |__     |  |   |  |       ||       |\n");
    printf("|__|\\__||___|__||_______||_______|  |___|  |___|___||_______|\n\n");

    printf("Справка по скриптовому языку Kristal v0.1.fix.17 rls (.krst):\n");
    printf("Команды:\n");
    printf("  stv name=\"data\" - Создает переменную name с значением data\n");
    printf("  stv name { data } - Создает переменную с многострочным значением\n");
    printf("  ourip['text' @var] - Выводит текст и значение переменной var\n");
    printf("  rd var ['prompt'] - Читает ввод с клавиатуры в переменную var\n");
    printf("  ifi var = value [ code ] j [ code ] - Если var равно value, выполняет первый блок, иначе второй\n");
    printf("  arc['archive.artr' 'file1' 'file2'] - Архивирует файлы или папки в archive.artr\n");
    printf("  unarc['archive.artr' 'dir'] - Распаковывает архив в папку dir\n");
    printf("  wf['file' 'data'] - Записывает данные (или @var) в файл\n");
    printf("  rf['file'] - Читает и выводит содержимое файла с переносом строки\n");
    printf("  lvl['file' 'p|u|k'] - Устанавливает уровень доступа (p, u, k) для файла\n");
    printf("  clear - Очищает экран\n");
    printf("  math var = a op b - Выполняет операцию (op: +, -, *, /) и сохраняет в var\n");
    printf("  grep['pattern' 'file'] - Ищет строки с pattern в файле\n");
    printf("  cat['file'] - Выводит содержимое файла\n");
    printf("  help - Выводит эту справку\n");
    printf("Флаги:\n");
    printf("  -e - Записывает ошибки в error.txt\n");
    printf("Пример запуска: ./kristal [-e] script.krst\n");
}

void process_kristal_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        log_error("Ошибка открытия файла %s!\n", filename);
        process_help();
        return;
    }

    printf(" __  __  ______  _______  _______  _______  _______  _____\n");
    printf("|  |/  ||   __ \\|_     _||     __||_     _||   _   ||     |_\n");
    printf("|     < |      < _|   |_ |__     |  |   |  |       ||       |\n");
    printf("|__|\\__||___|__||_______||_______|  |___|  |___|___||_______|\n\n");


    char line[256];
    while (fgets(line, 256, file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        if (strncmp(line, "stv", 3) == 0) {
            process_stv(line);
        } else if (strncmp(line, "ourip", 5) == 0) {
            process_ourip(line);
        } else if (strncmp(line, "rd", 2) == 0) {
            process_rd(line);
        } else if (strncmp(line, "ifi", 3) == 0) {
            process_ifi(line, file);
        } else if (strncmp(line, "arc", 3) == 0) {
            process_arc(line);
        } else if (strncmp(line, "unarc", 5) == 0) {
            process_unarc(line);
        } else if (strncmp(line, "wf", 2) == 0) {
            process_wf(line);
        } else if (strncmp(line, "rf", 2) == 0) {
            process_rf(line);
        } else if (strncmp(line, "lvl", 3) == 0) {
            process_lvl(line);
        } else if (strncmp(line, "clear", 5) == 0) {
            process_clear(line);
        } else if (strncmp(line, "math", 4) == 0) {
            process_math(line);
        } else if (strncmp(line, "grep", 4) == 0) {
            process_grep(line);
        } else if (strncmp(line, "cat", 3) == 0) {
            process_cat(line);
        } else if (strncmp(line, "help", 4) == 0) {
            process_help();
        } else {
            log_error("Неизвестная команда: %s\n", line);
        }
    }
    fclose(file);
    if (error_file) {
        fclose(error_file);
        error_file = NULL;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "help") == 0) {
        process_help();
        return 0;
    }

    int opt;
    while ((opt = getopt(argc, argv, "e")) != -1) {
        switch (opt) {
            case 'e':
                error_to_file = 1;
                break;
            default:
                printf("Использование: %s [-e] <файл.krst>\n", argv[0]);
                process_help();
                return 1;
        }
    }

    if (optind >= argc) {
        printf("Использование: %s [-e] <файл.krst>\n", argv[0]);
        process_help();
        return 1;
    }

    process_kristal_file(argv[optind]);
    return 0;
}