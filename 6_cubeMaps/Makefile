TP = ../thirdParty/

app_objects = tmp/baseApp.o tmp/glContext.o tmp/flyThroughCamera.o tmp/objParser.o tmp/texture.o
glad_objects = tmp/glad.o
imgui_objects = tmp/imgui.o tmp/imgui_draw.o tmp/imgui_widgets.o
imgui_impl_objects = tmp/imgui_impl_glfw.o tmp/imgui_impl_opengl3.o

mingw: $(app_objects) $(glad_objects) $(imgui_objects) $(imgui_impl_objects);
	g++ main.cpp \
	$(app_objects) $(glad_objects) $(imgui_objects) $(imgui_impl_objects) \
	-o main.exe \
	-I$(TP) -I$(TP)glm -I$(TP)imgui \
	-lglfw3 -lgdi32 -lopengl32 \
	-Wall -Wextra

$(app_objects):
	g++ -c ../common/$(*F).cpp -o $@ -I$(TP) -I$(TP)glm -I$(TP)imgui

$(glad_objects):
	g++ -c $(TP)glad/$(*F).c -o $@

$(imgui_objects):
	g++ -c $(TP)imgui/$(*F).cpp -o $(@) -I$(TP) -I$(TP)imgui

$(imgui_impl_objects):
	g++ -c $(TP)imgui/examples/$(*F).cpp -o $(@) -I$(TP) -I$(TP)imgui

