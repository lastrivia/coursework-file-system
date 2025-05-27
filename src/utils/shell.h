#ifndef SHELL_H
#define SHELL_H

#include <readline/readline.h>
#include <readline/history.h>

namespace cs2313 {

    static constexpr char
            STYLE_RESET[] = "\033[0m", STYLE_BOLD[] = "\033[1m", STYLE_DIM[] = "\033[2m", STYLE_ITALIC[] = "\033[3m",
            STYLE_UNDERLINE[] = "\033[4m", STYLE_BLINK[] = "\033[5m", STYLE_INVERT[] = "\033[7m", STYLE_HIDE[] = "\033[8m",
            STYLE_BLACK[] = "\033[30m", STYLE_RED[] = "\033[31m", STYLE_GREEN[] = "\033[32m", STYLE_YELLOW[] = "\033[33m",
            STYLE_BLUE[] = "\033[34m", STYLE_MAGENTA[] = "\033[35m", STYLE_CYAN[] = "\033[36m", STYLE_WHITE[] = "\033[37m",
            STYLE_BLACK_H[] = "\033[90m", STYLE_RED_H[] = "\033[91m", STYLE_GREEN_H[] = "\033[92m", STYLE_YELLOW_H[] = "\033[93m",
            STYLE_BLUE_H[] = "\033[94m", STYLE_MAGENTA_H[] = "\033[95m", STYLE_CYAN_H[] = "\033[96m", STYLE_WHITE_H[] = "\033[97m";

    template<typename T, typename... U>
    std::string styled(const T &text, const U &... styles) {
        std::ostringstream oss;
        ((oss << styles), ...);
        oss << text << STYLE_RESET;
        return oss.str();
    }

    inline void shell_init() {
        using_history();
    }

    inline std::string shell_input(const std::string &prompt) {
        char *input = readline(prompt.c_str());
        if (!input)
            return "";

        std::string line = input;
        if (!line.empty()) {
            for (int i = 0; i < history_length; ++i) {
                if (history_get(history_base + i) && line == history_get(history_base + i)->line) {
                    remove_history(i);
                    break;
                }
            }
            add_history(line.c_str());
        }

        free(input);
        return line;
    }

}
#endif
