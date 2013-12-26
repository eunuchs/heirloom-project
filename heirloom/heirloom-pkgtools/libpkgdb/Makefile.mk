.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(WARN) $<


OBJ = attach.o auth.o btree.o btree_rb.o build.o copy.o delete.o \
	encode.o expr.o func.o hash.o insert.o main.o os.o pager.o \
	pragma.o printf.o random.o select.o shell.o table.o tokenize.o \
	trigger.o update.o util.o vacuum.o vdbe.o where.o parse.o opcodes.o

OBJ =

GENERATED = def_ptr_sz config.h parse.h parse.c lempar.c lemon opcodes.c opcodes.h

all: libpkgdb.a

libpkgdb.a: $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

config.h: def_ptr_sz
	./def_ptr_sz >$@
	echo >> $@

def_ptr_sz: tool/def_ptr_sz.c
	$(CC) -o $@ tool/def_ptr_sz.c

lemon: tool/lemon.c
	$(CC) -o $@ tool/lemon.c

lempar.c: tool/lempar.c
	cp -f tool/lempar.c .

parse.h: parse.c

parse.c: parse.y lemon lempar.c
	./lemon parse.y
	rm -f parse.out y.tab.c yacc.acts yacc.debug yacc.tmp

opcodes.c: vdbe.c
	echo '/* Automatically generated file.  Do not edit */' >opcodes.c
	echo 'char *sqliteOpcodeNames[] = { "???", ' >>opcodes.c
	grep '^case OP_' vdbe.c | \
		sed -e 's/^.*OP_/  "/' -e 's/:.*$$/", /' >>opcodes.c
	echo '};' >>opcodes.c

opcodes.h: vdbe.h
	echo '/* Automatically generated file.  Do not edit */' >opcodes.h
	grep '^case OP_' vdbe.c | \
		sed -e 's/://' | \
		awk '{printf "#define %-30s %3d\n", $$2, ++cnt}' >>opcodes.h

install: all

clean:
	rm -f libpkgdb.a $(OBJ) $(GENERATED) core log *~

mrproper: clean

attach.o: attach.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
auth.o: auth.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
btree.o: btree.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h pager.h
btree_rb.o: btree_rb.c btree.h sqliteInt.h config.h sqlite.h hash.h \
  vdbe.h opcodes.h parse.h
build.o: build.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
copy.o: copy.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
delete.o: delete.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
encode.o: encode.c
expr.o: expr.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
func.o: func.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
hash.o: hash.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
insert.o: insert.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
main.o: main.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h os.h
os.o: os.c os.h sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
pager.o: pager.c os.h sqliteInt.h config.h sqlite.h hash.h vdbe.h \
  opcodes.h parse.h btree.h pager.h
pragma.o: pragma.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
printf.o: printf.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
random.o: random.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h os.h
select.o: select.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
shell.o: shell.c sqlite.h
table.o: table.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
tokenize.o: tokenize.c sqliteInt.h config.h sqlite.h hash.h vdbe.h \
  opcodes.h parse.h btree.h os.h
trigger.o: trigger.c sqliteInt.h config.h sqlite.h hash.h vdbe.h \
  opcodes.h parse.h btree.h
update.o: update.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
util.o: util.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
vacuum.o: vacuum.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h os.h
vdbe.o: vdbe.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h os.h
where.o: where.c sqliteInt.h config.h sqlite.h hash.h vdbe.h opcodes.h \
  parse.h btree.h
