echo WIN_LIB =  -lgdi32     > config.mk
echo WIN_LIB += -lcomctl32 >> config.mk
echo WIN_LIB += -lcomdlg32 >> config.mk
echo WIN_LIB += -ldinput8  >> config.mk
echo WIN_LIB += -ldxguid   >> config.mk
echo WIN_LIB += -lopengl32 >> config.mk
echo CONF_LFLAGS = $(WIN_LIB)              >> config.mk
echo EXESUFFIX = .exe                      >> config.mk
echo EXTRAOBJ = obj/resource$(OBJSUFFIX).o >> config.mk
echo RC = windres                          >> config.mk
echo RCFLAGS =                             >> config.mk
echo obj/resource$(OBJSUFFIX).o: ico/*     >> config.mk
echo 	$(RC) $(RCFLAGS) ico/minir.rc obj/resource$(OBJSUFFIX).o >> config.mk
