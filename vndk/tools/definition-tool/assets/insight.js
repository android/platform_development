(function (root, factory) {
    if (typeof define === 'function' && define.amd) {
        define([], factory);
    } else if (typeof module === 'object' && module.exports) {
        module.exports = factory();
    } else {
        root.insight = factory();
    }
} (this, function () {
    'use strict';

    var document, strs, mods, domPathInput, domDisplay;

    function init(doc, strings, modules) {
        document = doc;
        strs = strings;
        mods = modules;

        document.addEventListener('DOMContentLoaded', function (evt) {
            createControl();
            createDisplay();
        });
    }

    function createControlLabel(text) {
        var domTd, domStrong;
        domStrong = document.createElement('strong');
        domStrong.appendChild(document.createTextNode(text));
        domTd = document.createElement('td');
        domTd.appendChild(domStrong);
        return domTd;
    }

    function createControl() {
        var domTable, domTBody, domTr, domTd, domUl, domLi, domA, domStrong,
            domText, domForm, domBtn, tags;

        domTable = document.createElement('table');
        document.body.appendChild(domTable);

        domTBody = document.createElement('tbody');
        domTable.appendChild(domTBody);

        domTr = document.createElement('tr');
        domTBody.appendChild(domTr);

        domTr.appendChild(createControlLabel('Selection:'));

        domTd = document.createElement('td');
        domTr.appendChild(domTd);
        domUl = document.createElement('ul');
        domUl.className = 'menu';
        domTd.appendChild(domUl);

        domLi = document.createElement('li');
        domLi.appendChild(createLinkDOM('All', onSelectAll));
        domUl.appendChild(domLi);

        domLi = document.createElement('li');
        domLi.appendChild(createLinkDOM('32-bit', onSelect32bit));
        domUl.appendChild(domLi);

        domLi = document.createElement('li');
        domLi.appendChild(createLinkDOM('64-bit', onSelect64bit));
        domUl.appendChild(domLi);

        domLi = document.createElement('li');
        domLi.appendChild(createLinkDOM('None', onClear));
        domUl.appendChild(domLi);

        tags = collectAllTags();
        if (tags.size != 0) {
            domTr = document.createElement('tr');
            domTBody.appendChild(domTr);

            domTr.appendChild(createControlLabel('Add by Tags:'));

            domTd = document.createElement('td');
            domTr.appendChild(domTd);

            domUl = document.createElement('ul');
            domUl.className = 'menu';
            domTd.appendChild(domUl);

            for (let tag of tags) {
                domLi = document.createElement('li');
                domLi.appendChild(createLinkDOM(strs[tag], function (evt) {
                    evt.preventDefault(true);
                    addModulesByTag(tag);
                }));
                domUl.appendChild(domLi);
            }
        }

        domTr = document.createElement('tr');
        domTBody.appendChild(domTr);

        domTr.appendChild(createControlLabel('Add by Path:'));
        domTd = document.createElement('td');
        domTr.appendChild(domTd);

        domForm = document.createElement('form');
        domForm.addEventListener('submit', onSubmitPath);
        domTd.appendChild(domForm);

        domPathInput = document.createElement('input');
        domPathInput.type = 'text';
        domForm.appendChild(domPathInput);

        domBtn = document.createElement('input');
        domBtn.type = 'submit';
        domBtn.value = 'Add';
        domForm.appendChild(domBtn);

        document.body.appendChild(document.createElement('hr'));
    }

    function createDisplay() {
        var domTable;
        domDisplay = document.createElement('tbody');
        domDisplay.id = 'module_tbody';
        domTable  = document.createElement('table');
        domTable.appendChild(domDisplay);
        document.body.appendChild(domTable);
        createDisplayHeader();
        addAllModules();
    }

    function createDisplayHeader() {
        var domTr, domTh, domText, label, labels;

        labels = [
            'Name',
            'Tags',
            'Dependencies (Direct + Indirect)',
            'Users',
        ];

        domTr = document.createElement('tr');
        domDisplay.appendChild(domTr);
        for (label of labels) {
            domTh = document.createElement('th');
            domTh.appendChild(document.createTextNode(label));
            domTr.appendChild(domTh);
        }
    }

    function createLinkDOM(text, onClick) {
        var domA = document.createElement('a');
        domA.setAttribute('href', '#');
        domA.addEventListener('click', onClick);
        domA.appendChild(document.createTextNode(text));
        return domA;
    }

    function onSubmitPath(evt) {
        var path, mod;
        evt.preventDefault();
        path = domPathInput.value;
        domPathInput.value = '';
        for (mod in mods) {
            if (getModuleName(mod) == path) {
                addModule(mod);
                return;
            }
        }
        alert('Path not found: ' + path);
    }

    function onSelectAll(evt) {
        evt.preventDefault(true);
        removeAllModules();
        addAllModules();
    }

    function onSelect32bit(evt) {
        evt.preventDefault(true);
        removeAllModules();
        addModulesByFilter(function (mod) {
            return getModuleClass(mod) == 32;
        });
    }

    function onSelect64bit(evt) {
        evt.preventDefault(true);
        removeAllModules();
        addModulesByFilter(function (mod) {
            return getModuleClass(mod) == 64;
        });
    }

    function onClear(evt) {
        evt.preventDefault(true);
        removeAllModules();
    }

    function onModuleLinkClicked(evt) {
        addModule(evt.target.getAttribute('data-mod-id'));
    }

    function removeAllModules() {
        var domOldDisplay = domDisplay;
        domDisplay = domOldDisplay.cloneNode(false);
        domOldDisplay.parentNode.replaceChild(domDisplay, domOldDisplay);
        createDisplayHeader();
    }

    function addAllModules() {
        addModulesByFilter(function (mod) { return true; });
    }

    function addModulesByTag(tag) {
        addModulesByFilter(function (mod) {
            return isModuleTagged(mod, tag);
        });
    }

    function addModulesByFilter(pred) {
        for (let i in mods) {
            if (pred(i)) {
                addModule(i);
            }
        }
    }

    function createModuleLink(mod) {
        let domA = document.createElement('a');
        domA.setAttribute('href', '#mod_' + mod);
        domA.setAttribute('data-mod-id', mod);
        domA.addEventListener('click', onModuleLinkClicked);
        domA.appendChild(document.createTextNode(getModuleName(mod)));
        return domA;
    }

    function addModuleRelations(parent, label, mods) {
        let domH2 = document.createElement('h2');
        domH2.appendChild(document.createTextNode(label));
        parent.appendChild(domH2);

        let domOl = document.createElement('ol');
        parent.appendChild(domOl);

        for (let mod of mods) {
            let domLi = document.createElement('li');
            domLi.appendChild(createModuleLink(mod));
            domOl.appendChild(domLi);
        }
    }

    function addModule(mod) {
        var domTr, domTd, domP, domText, domDiv, domOl, domH2, numDeps,
            numUsers, deps, users;

        let modId = 'mod_'  + mod;

        if (document.getElementById(modId)) {
            return;
        }

        domTr = document.createElement('tr');
        domTr.id = modId;
        domDisplay.appendChild(domTr);

        domTd = document.createElement('td');
        domTd.appendChild(document.createTextNode(getModuleName(mod)));
        domTr.appendChild(domTd);

        domTd = document.createElement('td');
        domTr.appendChild(domTd);
        for (let tag of getModuleTags(mod)) {
            domP = document.createElement('p');
            domP.appendChild(document.createTextNode(strs[tag]));
            domTd.appendChild(domP);
        }

        numDeps = getModuleNumDeps(mod);
        domTd = document.createElement('td');
        domTd.appendChild(
                document.createTextNode(numDeps[0] + ' + ' + numDeps[1]));
        domTr.appendChild(domTd);

        deps = getModuleDeps(mod);
        if (deps.length > 0) {
            addModuleRelations(domTd, 'Direct', deps[0]);
            for (let i = 1; i < deps.length; ++i) {
                addModuleRelations(domTd, 'Indirect #' + i, deps[i]);
            }
        }

        numUsers = getModuleNumUsers(mod);
        domTd = document.createElement('td');
        domTd.appendChild(document.createTextNode(numUsers));
        domTr.appendChild(domTd);
        if (numUsers > 0) {
            addModuleRelations(domTd, 'Direct', getModuleUsers(mod));
        }

        domDisplay.appendChild(domTr);
    }

    function getModuleName(mod) {
        return strs[mods[mod][0]];
    }

    function getModuleClass(mod) {
        return mods[mod][1];
    }

    function getModuleTags(mod) {
        return mods[mod][2];
    }

    function getModuleDeps(mod) {
        return mods[mod][3];
    }

    function getModuleUsers(mod) {
        return mods[mod][4];
    }

    function getModuleNumDeps(mod) {
        let direct = 0;
        let indirect = 0;
        let deps = getModuleDeps(mod);
        if (deps.length > 0) {
            direct = deps[0].length;
            for (let i = 1; i < deps.length; ++i) {
                indirect += deps[i].length;
            }
        }
        return [direct, indirect];
    }

    function getModuleNumUsers(mod) {
        return getModuleUsers(mod).length;
    }

    function isModuleTagged(mod, tag) {
        var i, tags;
        tags = getModuleTags(mod);
        for (i = 0; i < tags.length; ++i) {
            if (tags[i] == tag) {
                return true;
            }
        }
        return false;
    }

    function collectAllTags() {
        var res = new Set();
        for (let mod in mods) {
            for (let tag of getModuleTags(mod)) {
                res.add(tag);
            }
        }
        return res;
    }

    return {
        'init': init,
    };
}));
