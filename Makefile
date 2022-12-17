#HAILER Makefile

HAILER_LIB = bin/libhailer.so
SERVER_EXE = bin/hailer_server
HAILER_CLI = bin/hailer_cli

LIB_SRC_DIR = src
LIB_OBJ_DIR = src/obj
LIB_SRCS = ${LIB_SRC_DIR}/hailer.c
LIB_OBJS := $(patsubst $(LIB_SRC_DIR)/%.c, $(LIB_OBJ_DIR)/%.o, $(LIB_SRCS))
LIB_DEPENDS := ${patsubst ${LIB_SRC_DIR}/%.c, ${LIB_OBJ_DIR}/%.d, ${LIB_SRCS}}

SERVER_SRC_DIR = src
SERVER_OBJ_DIR = src/obj
SERVER_SRCS = ${SERVER_SRC_DIR}/hailer_server.c ${SERVER_SRC_DIR}/hailer_peer_discovery.c
SERVER_OBJS := $(patsubst $(SERVER_SRC_DIR)/%.c, $(SERVER_OBJ_DIR)/%.o, $(SERVER_SRCS))
SERVER_DEPENDS := ${patsubst ${SERVER_SRC_DIR}/%.c, ${SERVER_OBJ_DIR}/%.d, ${SERVER_SRCS}}

HAILER_CLI_SRC_DIR = src
HAILER_CLI_SRC = ${HAILER_CLI_SRC_DIR}/hailer_cli.c

LIBS := -lhailer -lpthread -ljson-c

TEST_APPS := test-app/test_app*.exe

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g -ggdb3
LDFLAGS = -shared
RM = rm -f

.PHONY: all clean install

all: ${HAILER_LIB} ${SERVER_EXE} ${HAILER_CLI} install

$(HAILER_LIB): $(LIB_OBJS)
	$(CC) ${LDFLAGS} $^ -o $@
	install -d /usr/lib/
	install -m 644 $(HAILER_LIB) /usr/lib/

$(SERVER_EXE): $(SERVER_OBJS) ${HAILER_LIB}
	$(CC) ${CFLAGS} ${SERVER_OBJS} ${LIBS} -o $@

$(HAILER_CLI): $(HAILER_CLI_SRC) ${HAILER_LIB}
	$(CC) ${CFLAGS} ${HAILER_CLI_SRC} ${LIBS} -o $@

-include ${LIB_DEPENDS} ${SERVER_DEPENDS}

${LIB_OBJ_DIR}/%.o: ${LIB_SRC_DIR}/%.c Makefile ${LIB_SRC_DIR}/include/%.h | src/obj
	${CC} -MMD -MP -c $< -o $@

${SERVER_OBJ_DIR}/%.o: ${SERVER_SRC_DIR}/%.c Makefile ${SERVER_SRC_DIR}/include/%.h | src/obj
	${CC} -MMD -MP -c $< -o $@

src/obj:
	mkdir src/obj

install: $(SERVER_EXE)
	install -d /usr/bin/
	install -m 744 $(SERVER_EXE) /usr/bin/
	install -m 744 $(HAILER_CLI) /usr/bin/

clean:
	-${RM} ${HAILER_LIB} $(SERVER_EXE) $(HAILER_CLI) $(LIB_OBJS) ${LIB_DEPENDS} ${SERVER_OBJS} ${SERVER_DEPENDS} ${TEST_APPS}