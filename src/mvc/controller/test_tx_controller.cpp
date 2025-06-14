#include "test_tx_controller.h"
#include "mvc/dao/test_tx_dao.h"
#include "util/utils.h"
#include "log/logs.h"

// #include ""

const std::string BASE_TEXT = "[TestTxController] ";

void TestTxController::handle(HttpRequest& req, HttpResponse& res) {
    LOG_INFO(BASE_TEXT + "in handle.");
    // const std::unordered_map<std::string, std::string>& formData = req.getFormData();
    // std::string username = formData.at("username");
    // std::string age = formData.at("age");

    const Json json = req.getJson();
    std::string username = json["username"];
    std::string age = json["age"];

    bool success = TextTxDao::insertUser(username, std::stoi(age));

    if (success) {
        res.sendRESTfulJson(200, "事务提交成功", Json({}));
    }
    else {
        res.sendRESTfulJson(500, "事务失败，已回滚", Json({}));
    }
}