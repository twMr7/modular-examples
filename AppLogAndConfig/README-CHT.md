## AppLoadPlugin

本範例修改自 [Poco](http://pocoproject.org/) 的 "SampleApp" 專案。 範例中展示了封裝在 **Application** 類別的 *config*，*logger*，和命令列 *defineOptions* 方法，以及如何優雅地用來建立功能豐富的應用程式。

展示重點：

1. 在 **wmain()** 裡控制繼承自 Poco::Application 的主要應用程式流程。
2. 如何載入預設設定檔，以及額外的設定檔。
3. 如何透過 **Application::defineOptions()** 方法處理命令參數選項。
4. 如何透過命令參數指定或修改 config 設定值。試試輸入命令參數 **/d configFile=custom.ini**，以及命令參數 **/f custom.ini**。
5. 在 **Application::initialize()** 如何載入預設或指定的設定檔。
6. 設定檔中包含針對 **logger** 的不同設定，請試著修改專案中的兩個ini設定檔內容，以及試著指定程式載入不同設定檔，試試參數改變的差異。
7. 注意 **logger** 在程式中如何輸出不同等級的日誌？
8. 叫用 **logger().trace()** 方法 和 **poco_trace()** 巨集有何差異？
