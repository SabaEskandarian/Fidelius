function test1() {
    local_storage_data["key1"] = "value1";
    value1 = local_storage_data["key"];
    local_storage_data = null;
    value2 = local_storage_data["key"];
    output = "getItem: value1=" + value1 + ", after clear:" + "value1=" + value2;
}

function test2a() {
    localStorage.putItem("key1", "value1");
}

function test2b() {
    value1 = localStorage.getItem("key1")
    output = "getItem: value1=" + value1;
}