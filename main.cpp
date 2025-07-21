#include <iostream>
#include <windows.h>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
constexpr auto WIKI_API_URL = "https://zh.minecraft.wiki/api.php";
using json = nlohmann::json; // 简化命名空间

int main() {
    // 设置控制台编码为UTF-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

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
		json LOG_TYPE_MAP = config["log_type_map"];
		json LOG_ACTION_MAP = config["log_action_map"];
		json AF_ACTION_MAP = config["af_action_map"];
		json AF_RESULT_MAP = config["af_result_map"];

        // 加载提示信息
        std::string LOGIN_TOKEN_FAIL = config["login_token_fail"];
        std::string LOGIN_TOKEN_SUCCESS = config["login_token_success"];
        std::string LOGIN_FAIL = config["login_fail"];
        std::string LOGIN_SUCCESS = config["login_success"];
        std::string INITIAL_FAIL = config["initial_fail"];
        std::string STARTUP_SUCCESS = config["startup_success"];
        std::string REQUEST_FAIL = config["request_fail"];

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
            // ?action=query&format=json&list=recentchanges|abuselog&formatversion=2&rcprop=title|timestamp|ids|comment|user|loginfo|sizes&rcshow=!bot&rclimit=100&rctype=edit|new|log|external&afllimit=100&aflprop=ids|user|title|action|result|timestamp|revid|filter
            cpr::Payload initial_params = {
                {"action", "query"},
                {"format", "json"},
                {"list", "recentchanges|abuselog"},
                {"formatversion", "2"},
                {"rcprop", "timestamp|ids"},
                {"rcshow", "!bot"},
                {"rclimit", "1"},
                {"rctype", "edit|new|log|external"},
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

                // 打印新内容
                // API沙盒：https://zh.minecraft.wiki/w/Special:API%E6%B2%99%E7%9B%92#action=query&format=json&list=recentchanges%7Cabuselog&formatversion=2&rcprop=title%7Ctimestamp%7Cids%7Ccomment%7Cuser%7Cloginfo%7Csizes&rcshow=!bot&rclimit=1&rctype=log%7Cedit%7Cnew&afllimit=1&aflprop=ids%7Cuser%7Ctitle%7Caction%7Cresult%7Ctimestamp%7Crevid%7Cfilter
                if (!new_rc_items.empty()) {
					std::reverse(new_rc_items.begin(), new_rc_items.end());
                    std::cout << "New Recent Changes:" << std::endl;
                    for (const auto& item : new_rc_items) {
                        if (item["type"].get<std::string>() == "log") {
                            std::cout << LOG_ACTION_MAP[item["logtype"].get<std::string>()] << ": " << std::endl;
                        } else {
                            std::cout << "Edit: ";
						}
                        std::cout << "Title: " << item["title"].get<std::string>()
                                  << ", Timestamp: " << item["timestamp"].get<std::string>()
                                  << ", User: " << item["user"].get<std::string>()
                                  << ", Comment: " << item["comment"].get<std::string>() << std::endl;
                    }
                    last_rc_timestamp = new_rc_items.back()["timestamp"].get<std::string>();
                    last_rcid = new_rc_items.back()["rcid"].get<int>();
				}
                if (!new_afl_items.empty()) {
                    std::reverse(new_afl_items.begin(), new_afl_items.end());
                    std::cout << "New Abuse Log:" << std::endl;
                    for (const auto& item : new_afl_items) {
                        std::cout << "Title: " << item["title"].get<std::string>()
                                  << ", Timestamp: " << item["timestamp"].get<std::string>()
                                  << ", User: " << item["user"].get<std::string>()
                                  << ", Action: " << item["action"].get<std::string>()
                                  << ", Result: " << item["result"].get<std::string>() << std::endl;
                    }
                    last_afl_timestamp = new_afl_items.back()["timestamp"].get<std::string>();
					last_afl_id = new_afl_items.back()["id"].get<int>();
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
