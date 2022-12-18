#HAILER Makefile

HAILER_LIB = bin/libhailer.so
SERVER_EXE = bin/hailer_server

LIB_SRC_DIR = src
LIB_OBJ_DIR = src/obj
LIB_SRCS = ${LIB_SRC_DIR}/hailer.c
LIB_OBJS := $(patsubst $(LIB_SRC_DIR)/%.c, $(LIB_OBJ_DIR)/%.o, $(LIB_SRCS))
LIB_DEPENDS := ${patsubst ${LIB_SRC_DIR}/%.c, ${LIB_OBJ_DIR}/%.d, ${LIB_SRCS}}

SERVER_SRC_DIR = src
SERVER_OBJ_DIR = src/obj
SERVER_SRCS = ${SERVER_SRC_DIR}/hailer_server.c
SERVER_OBJS := $(patsubst $(SERVER_SRC_DIR)/%.c, $(SERVER_OBJ_DIR)/%.o, $(SERVER_SRCS))
SERVER_DEPENDS := ${patsubst ${SERVER_SRC_DIR}/%.c, ${SERVER_OBJ_DIR}/%.d, ${SERVER_SRCS}}
LIBS := -lhailer

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g -ggdb3
LDFLAGS = -shared
RM = rm -f

.PHONY: all clean install

all: ${HAILER_LIB} ${SERVER_EXE} install

$(HAILER_LIB): $(LIB_OBJS)
	$(CC) ${LDFLAGS} $^ -o $@
	install -d /usr/lib/
	install -m 644 $(HAILER_LIB) /usr/lib/

$(SERVER_EXE): $(SERVER_OBJS) ${HAILER_LIB}
	$(CC) ${CFLAGS} ${SERVER_OBJS} ${LIBS} -o $@

-include ${LIB_DEPENDS} ${SERVER_DEPENDS}

${LIB_OBJ_DIR}/%.o: ${LIB_SRC_DIR}/%.c Makefile ${LIB_SRC_DIR}/include/%.h | src/obj
	${CC} -MMD -MP -c $< -o $@

${SERVER_OBJ_DIR}/%.o: ${SERVER_SRC_DIR}/%.c Makefile ${SERVER_SRC_DIR}/include/%.h | src/obj
	${CC} -MMD -MP -c $< -o $@

src/obj:
	mkdir src/obj

install: $(SERVER_EXE)
	install -d /usr/bin/
	install -m 644 $(SERVER_EXE) /usr/bin/

clean:
	-${RM} ${HAILER_LIB} $(SERVER_EXE) $(LIB_OBJS) ${LIB_DEPENDS} ${SERVER_OBJS} ${SERVER_DEPENDS}