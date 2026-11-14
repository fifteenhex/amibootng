#ifndef AMIBOOTNG_VERSION_H
#define AMIBOOTNG_VERSION_H

#define AMIBOOTNG_VERSION       "1.0"
#define AMIBOOTNG_DATE          "14.11.2026"

/* the version string AmigaOS's Version command and the Installer look for */
#define VERSTAG(name)                                                   \
    static const char verstag[] __attribute__((used)) =                 \
        "$VER: " name " " AMIBOOTNG_VERSION " (" AMIBOOTNG_DATE ")"

#endif
