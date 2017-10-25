

#include "io_config.h"
#include "io_handler.h"

////////////////////////////////////////////////////////////////////////////////
#ifdef NPX_WINDOWS
int disable_console_menu(){
	HWND hwnd = GetConsoleWindow();
	if (!hwnd){
		return 0;
	}
	DeleteMenu(GetSystemMenu(hwnd, FALSE), SC_CLOSE, MF_BYCOMMAND);
	return DrawMenuBar(hwnd), 0;
}
#else
int disable_console_menu(){return 0;}
#endif
#include "algo/srp6.h"

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	//srp6 tttt, client, server;
	//for (int i = 0; i < 10; i++){
	//    unsigned int t1 = npx_tick_count();
	//    const std::string token = tttt.token("abc");
	//    const std::string request = server.request(token);
	//    const std::string response = client.response(request, "abc");
	//    std::string data;
	//    bool result1 = server.verify(response, data);
	//    bool result2 = client.verify(data);
	//    
	//    unsigned int t2 = npx_tick_count();
	//    if (result1 && result2){
	//        std::string key = client.get_key();
	//        io::stringc temp(key.c_str(), key.size());
	//        printf("%s(%d) %dms\r\n", temp.to_hexfmt().c_str(), temp.size(), t2 - t1);
	//    }
	//}
	//设置并初始化环境
	disable_console_menu();
	npx_set_path(npx_get_path());
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
	if (!config::load("../ini/config.ini")){
		PRINT("* Can't load config.ini.\r\n");
		return 0;
	}

#ifdef _DEBUG

	std::vector<int> vt;
	vt.push_back(1);
	vt.push_back(2);
	vt.push_back(3);
	std::swap(vt[0], vt[1]);


#endif

	unsigned short port = 10080;
	if (argc > 1){
		port = (unsigned short)atoi(argv[1]);
	}
	PRINT("* The port of bind local: %d\r\n", port);
	//绑定本地端口并运行
	const char *run_mode = config::get("mode_run_service");
	if (!run_mode){
		run_mode = "0";
	}
	http::run(handler(), 100, (void*)atoi(run_mode));
	if (http::listen(port, 0, 128)){
#ifdef _DEBUG
		_getch();
#else
		printf("\r\nPress <Ctrl + C> is exit.\r\n\r\n");
		npx_wait();
#endif
	} else {
		PRINT("* Bind failed for local port: %d\r\n\r\n", port);
	}
	http::stop();
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
