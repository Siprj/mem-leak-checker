TEMPLATE = subdirs

SUBDIRS += test-app \
    malloc-lib \
    malloc-lib-posprocess

HEADERS += \
    common-headers/common-enums.h

OTHER_FILES += \
    README.md \
    my-malloc.graphml
