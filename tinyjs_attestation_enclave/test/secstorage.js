function test1() {
    localStorage.putItem("key1", "value1");
    value1 = localStorage.getItem("key1");
    localStorage.clear();
    value2 = localStorage.getItem("key1")
    output = "getItem: value1=" + value1 + ", after clear:" + "value1=" + value2;
}

function test2a() {
    localStorage.putItem("key1", "value1");
}

function test2b() {
    value1 = localStorage.getItem("key1")
    output = "getItem: value1=" + value1;
}