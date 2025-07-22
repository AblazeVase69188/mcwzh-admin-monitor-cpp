#include <iostream>
#include <windows.h>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
constexpr auto WIKI_API_URL = "https://zh.minecraft.wiki/api.php";
constexpr auto WIKI_DIFF_URL = "https://zh.minecraft.wiki/?diff=";
constexpr auto WIKI_LOG_URL = "https://zh.minecraft.wiki/w/Special:%E6%97%A5%E5%BF%97/";
constexpr auto WIKI_AFL_URL = "https://zh.minecraft.wiki/w/Special:%E6%BB%A5%E7%94%A8%E6%97%A5%E5%BF%97/";
using json = nlohmann::json; // 简化命名空间

// 全局变量
json LOG_TYPE_MAP;
json LOG_ACTION_MAP;
json AF_ACTION_MAP;
json AF_RESULT_MAP;

// 定义UTF-8字符串
std::string LOGIN_TOKEN_FAIL(reinterpret_cast<const char*>(u8"登录令牌获取失败"));
std::string LOGIN_TOKEN_SUCCESS(reinterpret_cast<const char*>(u8"登录令牌获取成功"));
std::string LOGIN_FAIL(reinterpret_cast<const char*>(u8"登录失败"));
std::string LOGIN_SUCCESS(reinterpret_cast<const char*>(u8"登录成功"));
std::string INITIAL_FAIL(reinterpret_cast<const char*>(u8"获取初始数据失败"));
std::string STARTUP_SUCCESS(reinterpret_cast<const char*>(u8"启动成功"));
std::string REQUEST_FAIL(reinterpret_cast<const char*>(u8"获取数据失败"));
std::string ZH_COMMA(reinterpret_cast<const char*>(u8"，"));

// 函数声明
// 调整时间戳为UTC+8
static std::string adjust_timestamp(const std::string& timestamp) {
	std::string hour_str = timestamp.substr(11, 2);
	int hour = std::stoi(hour_str);
    hour = (hour + 8) % 24;
	std::string adjusted_str = (hour < 10 ? "0" : "") + std::to_string(hour) + timestamp.substr(13,6);
    return adjusted_str;
}
// 调整摘要输出
static void print_comment(const json& item) {
    const auto& comment = item["comment"].get<std::string>();
    if (!comment.empty()) {
        std::cout << comment;
    }
    else {
        std::cout << std::string(reinterpret_cast<const char*>(u8"（空）"));
    }
}
// 输出最近更改url
static void print_rc_url(const json& item) {
    if (item["revid"].get<int>() == 0) {
        std::cout << WIKI_LOG_URL << item["logtype"].get<std::string>();
    }
    else {
        std::cout << WIKI_DIFF_URL << item["revid"].get<int>();
    }
}
// 输出滥用日志url
static void print_afl_url(const json& item) {
    std::cout << WIKI_AFL_URL << item["id"].get<int>();
}
// 打印最近更改
static void printrc(const json& item) {
    if (item["type"].get<std::string>() == "log") {
        std::cout << std::string(reinterpret_cast<const char*>(u8"（")) << LOG_TYPE_MAP[item["logtype"].get<std::string>()].get<std::string>() << std::string(reinterpret_cast<const char*>(u8"）"));
        std::cout << adjust_timestamp(item["timestamp"].get<std::string>());
        std::cout << ZH_COMMA;
        std::cout << item["user"].get<std::string>() << std::string(reinterpret_cast<const char*>(u8"对")) << item["title"].get<std::string>() << std::string(reinterpret_cast<const char*>(u8"执行了")) << LOG_ACTION_MAP[item["logaction"].get<std::string>()].get<std::string>() << std::string(reinterpret_cast<const char*>(u8"操作"));
        std::cout << ZH_COMMA;
        std::cout << std::string(reinterpret_cast<const char*>(u8"摘要为"));
        print_comment(item);
        std::cout << std::endl;
		print_rc_url(item);
        std::cout << std::endl << std::endl;
    }
    else if (item["type"].get<std::string>() == "edit") {
        std::cout << adjust_timestamp(item["timestamp"].get<std::string>());
        std::cout << ZH_COMMA;
        std::cout << item["user"].get<std::string>() << std::string(reinterpret_cast<const char*>(u8"在")) << item["title"].get<std::string>() << std::string(reinterpret_cast<const char*>(u8"做出编辑"));
        std::cout << ZH_COMMA;
        std::cout << std::string(reinterpret_cast<const char*>(u8"字节更改为")) << item["newlen"].get<int>() - item["oldlen"].get<int>();
        std::cout << ZH_COMMA;
        std::cout << std::string(reinterpret_cast<const char*>(u8"摘要为"));
        print_comment(item);
        std::cout << std::endl;
        print_rc_url(item);
        std::cout << std::endl << std::endl;
    }
    else {
        std::cout << adjust_timestamp(item["timestamp"].get<std::string>());
        std::cout << ZH_COMMA;
        std::cout << item["user"].get<std::string>() << std::string(reinterpret_cast<const char*>(u8"创建")) << item["title"].get<std::string>();
        std::cout << ZH_COMMA;
        std::cout << std::string(reinterpret_cast<const char*>(u8"字节更改为")) << item["newlen"].get<int>() - item["oldlen"].get<int>();
        std::cout << ZH_COMMA;
        std::cout << std::string(reinterpret_cast<const char*>(u8"摘要为"));
        print_comment(item);
        std::cout << std::endl;
        print_rc_url(item);
        std::cout << std::endl << std::endl;
    }
}
// 打印滥用日志
static void printafl(const json& item) {
    std::cout << adjust_timestamp(item["timestamp"].get<std::string>());
    std::cout << ZH_COMMA;
    std::cout << item["user"].get<std::string>() << std::string(reinterpret_cast<const char*>(u8"在")) << item["title"].get<std::string>() << std::string(reinterpret_cast<const char*>(u8"执行操作")) << AF_ACTION_MAP[item["action"].get<std::string>()].get<std::string>() << std::string(reinterpret_cast<const char*>(u8"时触发了过滤器")) << item["filter"].get<std::string>();
    std::cout << ZH_COMMA;
	std::cout << std::string(reinterpret_cast<const char*>(u8"采取的行动为")) << AF_RESULT_MAP[item["result"].get<std::string>()].get<std::string>();
    std::cout << std::endl;
	print_afl_url(item);
	std::cout << std::endl << std::endl;
}

int main() {
    // 设置控制台为UTF-8编码
    SetConsoleOutputCP(CP_UTF8);

    // 读取并解析配置文件
    json config;
    std::ifstream file("config.json");
    if (!file.is_open()) {
        std::cerr << "config.json not found" << std::endl;
        return 1;
    }
    try {
        file >> config;
    
        // 加载配置参数
        std::string user_agent = config["user_agent"];
        std::string username = config["username"];
        std::string password = config["password"];

        // 加载系统消息
		LOG_TYPE_MAP = config["log_type_map"];
		LOG_ACTION_MAP = config["log_action_map"];
		AF_ACTION_MAP = config["af_action_map"];
		AF_RESULT_MAP = config["af_result_map"];

        // 登录
        cpr::Session session;
        session.SetUrl(cpr::Url{ WIKI_API_URL });
        session.SetHeader(cpr::Header{
            {"User-Agent", user_agent}
            });

        // 获取登录token
        session.SetParameters(
            cpr::Parameters{
                {"action", "query"},
                {"meta", "tokens"},
                {"type", "login"},
                {"format", "json"}
            }
        );
        cpr::Response login_token_response = session.Get();
        session.SetParameters(cpr::Parameters{});

        // 检查响应状态
        if (login_token_response.status_code != 200) {
            std::cerr << LOGIN_TOKEN_FAIL << login_token_response.status_code << std::endl;
            return 1;
        }

        // 解析JSON获取token
        json logintokenjson = json::parse(login_token_response.text);
        std::string login_token = logintokenjson["query"]["tokens"]["logintoken"];
        std::cout << LOGIN_TOKEN_SUCCESS << std::endl;

        // 提交登录信息
        session.SetPayload(
            cpr::Payload{
                {"action", "login"},
                {"lgname", username},
                {"lgpassword", password},
                {"lgtoken", login_token},
                {"format", "json"}
            }
        );
        cpr::Response login_response = session.Post();

        // 检查登录响应
        if (login_response.status_code != 200) {
            std::cerr << LOGIN_FAIL << login_response.status_code << std::endl;
            return 1;
        }

        // 解析登录结果
        json loginResult = json::parse(login_response.text);
        if (loginResult["login"]["result"] == "Success") {
            std::cout << LOGIN_SUCCESS << std::endl;

            // API基础参数
            // ?action=query&format=json&list=recentchanges|abuselog&formatversion=2&rcprop=title|timestamp|ids|comment|user|loginfo|sizes&rcshow=!bot&rclimit=100&rctype=edit|new|log&afllimit=100&aflprop=ids|user|title|action|result|timestamp|revid|filter
            cpr::Payload initial_params = {
                {"action", "query"},
                {"format", "json"},
                {"list", "recentchanges|abuselog"},
                {"formatversion", "2"},
                {"rcprop", "timestamp|ids"},
                {"rcshow", "!bot"},
                {"rclimit", "1"},
                {"rctype", "edit|new|log"},
                {"afllimit", "1"},
                {"aflprop", "ids|timestamp"}
            };

            // 获取最近的最近更改和滥用日志
            session.SetPayload(cpr::Payload{});
            session.SetPayload(initial_params);
            cpr::Response initial_response = session.Post();
            // 检查初始响应
            if (initial_response.status_code != 200) {
                std::cerr << INITIAL_FAIL << initial_response.status_code << std::endl;
                return 1;
            }
            json initial_data = json::parse(initial_response.text);
            std::string last_rc_timestamp = initial_data["query"]["recentchanges"][0]["timestamp"].get<std::string>();
            int last_rcid = initial_data["query"]["recentchanges"][0]["rcid"].get<int>();
            std::string last_afl_timestamp = initial_data["query"]["abuselog"][0]["timestamp"].get<std::string>();
            int last_afl_id = initial_data["query"]["abuselog"][0]["id"].get<int>();

            std::cout << STARTUP_SUCCESS << std::endl;

            while (1) {
                cpr::Payload current_params = {
                {"action", "query"},
                {"format", "json"},
                {"list", "recentchanges|abuselog"},
                {"formatversion", "2"},
                {"rcprop", "title|timestamp|ids|comment|user|loginfo|sizes"},
                {"rcshow", "!bot"},
                {"rclimit", "100"},
                {"rctype", "edit|new|log"},
                {"afllimit", "100"},
                {"aflprop", "ids|user|title|action|result|timestamp|revid|filter"},
                {"rcend", last_rc_timestamp},
                {"aflend", last_afl_timestamp}
                };

                std::this_thread::sleep_for(std::chrono::seconds(5));

                // 获取最近更改和滥用日志
                session.SetPayload(cpr::Payload{});
                session.SetPayload(current_params);

                cpr::Response current_response = session.Post();

                // 检查响应
                if (current_response.status_code != 200) {
                    std::cerr << REQUEST_FAIL << current_response.status_code << std::endl;
                    continue;
                }

                // 筛选新内容
                json current_data = json::parse(current_response.text);
                std::vector<json> new_rc_items;
                std::vector<json> new_afl_items;
                for (const auto& item : current_data["query"]["recentchanges"]) {
                    if (item["rcid"].get<int>() > last_rcid) {
						new_rc_items.push_back(item);
                    }
                }
                for (const auto& item : current_data["query"]["abuselog"]) {
                    if (item["id"].get<int>() > last_afl_id) {
						new_afl_items.push_back(item);
                    }
                }

				int is_new_rc = !new_rc_items.empty();
				int is_new_afl = !new_afl_items.empty();
                if (is_new_rc) {
                    std::reverse(new_rc_items.begin(), new_rc_items.end());
                    last_rc_timestamp = new_rc_items.back()["timestamp"].get<std::string>();
                    last_rcid = new_rc_items.back()["rcid"].get<int>();
                }
                if (is_new_afl) {
                    std::reverse(new_afl_items.begin(), new_afl_items.end());
                    last_afl_timestamp = new_afl_items.back()["timestamp"].get<std::string>();
                    last_afl_id = new_afl_items.back()["id"].get<int>();
                }

                // 打印新内容
                // API沙盒：https://zh.minecraft.wiki/w/Special:API%E6%B2%99%E7%9B%92#action=query&format=json&list=recentchanges%7Cabuselog&formatversion=2&rcprop=title%7Ctimestamp%7Cids%7Ccomment%7Cuser%7Cloginfo%7Csizes&rcshow=!bot&rclimit=1&rctype=log%7Cedit%7Cnew&afllimit=1&aflprop=ids%7Cuser%7Ctitle%7Caction%7Cresult%7Ctimestamp%7Crevid%7Cfilter
                if (is_new_rc && !is_new_afl)// 仅最近更改
                {
                    for (const auto& item : new_rc_items) {
						printrc(item);
                    }
                }
                else if (is_new_afl && !is_new_rc) // 仅滥用日志
                {
                    for (const auto& item : new_afl_items) {
						printafl(item);
                    }
                }
				else if (is_new_rc && is_new_afl) // 同时存在最近更改和滥用日志
                {
					/*int is_related = 0;
                    for (const auto& item : new_afl_items) {
                        if (item["revid"].get<int>()) {
							is_related = 1;
                        }
                    }*/
                    // 合并is_new_rc和is_new_afl
                    std::vector<json> merged_items;
                    merged_items.reserve(new_rc_items.size() + new_afl_items.size());
                    merged_items.insert(merged_items.end(), new_rc_items.begin(), new_rc_items.end());
                    merged_items.insert(merged_items.end(), new_afl_items.begin(), new_afl_items.end());

                    // 按时间顺序排序
                    std::sort(merged_items.begin(), merged_items.end(), [](const json& a, const json& b) {
                        return a["timestamp"].get<std::string>() < b["timestamp"].get<std::string>();
                    });

                    // 输出
                    for (const auto& item : merged_items) {
                        if (item.contains("type")) {
                            printrc(item);
                        }
                        else {
                            printafl(item);
                        }
                    }
                }
            }
        }
        else {
            std::cerr << LOGIN_FAIL << loginResult["login"].dump() << std::endl;
            return 1;
        }

    }
    catch (const json::exception& e) {
        std::cerr << "JSON error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
