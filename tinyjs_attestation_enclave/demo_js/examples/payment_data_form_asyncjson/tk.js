/*
 * Toolkit library
 */

function toDict(form) {
    dict = {};
    els = form.querySelectorAll("input, select, textarea");

    for (i = 0; i < els.length; ++i) {
        el = els[i];
        n = el.name;
        v = el.value;

        if (n) {
            dict[n] = v;
        }
    }

    return dict;
}
