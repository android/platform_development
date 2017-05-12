var diameter = 1280,
    radius = diameter / 2,
    innerRadius = radius - 240;

var cluster = d3.cluster()
    .size([360, innerRadius]);

var line = d3.radialLine()
    .curve(d3.curveBundle.beta(0.85))
    .radius(function(d) { return d.y; })
    .angle(function(d) { return d.x / 180 * Math.PI; });

var svg = d3.select("#dep_graph").append("svg")
    .attr("width", diameter + 200)
    .attr("height", diameter)
  .append("g")
    .attr("transform", "translate(" + (radius + 100) + "," + radius + ")");

var link = svg.append("g").selectAll(".link"),
    node = svg.append("g").selectAll(".node");

var selectedNode;

function show_list(dep_map, violate_libs) {
  function makeTitle(part_name) {
      var title = document.createElement('div');
      var name = document.createElement('h3');
      name.innerHTML = part_name;
      title.appendChild(name);
      return title;
  }
  function makeButton(lib_name, count) {
      var button = document.createElement('button');
      button.className = 'violate';
      button.innerHTML = lib_name + " (" + count + ")";
      button.onclick = function() {
          this.classList.toggle('active');
          var my_list = this.nextElementSibling;
          if (my_list.style.display === 'block') {
              my_list.style.display = 'none';
              selectedNode = undefined;
              resetclicked();
          } else {
              my_list.style.display = 'block';
              if (selectedNode) {
                  selectedNode.classList.toggle('active');
                  selectedNode.nextElementSibling.style.display = 'none';
              }
              selectedNode = button;
              mouseclicked(dep_map[lib_name]);
          }
      }
      return button;
  }
  function makeList(list_div, list) {
      for (var i = 0; i < list.length; i++) {
          list_div.appendChild(makeButton(list[i][0], list[i][1]));
          var children_div = document.createElement('div');
          var my_data = dep_map[list[i][0]];
          var violates = my_data.data.violates;
          for (var j = 0; j < violates.length; j++) {
              var dep_lib = document.createElement('div');
              var tag = dep_map[violates[j]].data.tag;
              dep_lib.className = 'violate-list';
              dep_lib.innerHTML = violates[j] + " ["
                    + tag.substring(tag.lastIndexOf(".") + 1) + "]";
              children_div.appendChild(dep_lib);
          }
          list_div.appendChild(children_div);
          children_div.style.display = 'none';
      }
  }
  var vilolate_list = document.getElementById('violate_list');
  if ("vendor.private.bin" in violate_libs) {
      var list = violate_libs["vendor.private.bin"];
      violate_list.appendChild(makeTitle("VENDOR (" + list.length + ")"));
      makeList(violate_list, list);
  }
  for (var tag in violate_libs) {
      if (tag === "vendor.private.bin") continue;
      var list = violate_libs[tag];
      if (tag === "system.private.bin") tag = "SYSTEM";
      else tag = tag.substring(tag.lastIndexOf(".")).upper();
      violate_list.appendChild(makeTitle(tag + " (" + list.length + ")"));
      makeList(violate_list, list);
  }
}

function show_result(classes, violate_libs) {
  var root = packageHierarchy(classes)
      .sum(function(d) { return d.size; });

  cluster(root);
  var dep_data = packageDepends(root.leaves());
  show_list(dep_data[1], violate_libs);
  link = link
    .data(dep_data[0])
    .enter().append("path")
      .each(function(d) { d.source = d[0], d.target = d[d.length - 1]; })
      .attr("class", function(d) { return d.allow ? "link" : "link--violate" })
      .attr("d", line);

  node = node
    .data(root.leaves())
    .enter().append("text")
    .attr("class", function(d) {
        return d.data.parent.parent.parent.key == "system"
            ? ( d.data.parent.parent.key == "system.public" ? "node--sys-pub" : "node--sys-pri" )
            : "node";
    })
      .attr("dy", "0.31em")
      .attr("transform", function(d) { return "rotate(" + (d.x - 90) + ")translate(" + (d.y + 8) + ",0)" + (d.x < 180 ? "" : "rotate(180)"); })
      .attr("text-anchor", function(d) { return d.x < 180 ? "start" : "end"; })
      .text(function(d) { return d.data.key; })
      .on("click", mouseclicked);
  document.getElementById('reset_btn').onclick = resetclicked;
}

function resetclicked() {
  if (selectedNode) {
      selectedNode.classList.toggle('active');
      selectedNode.nextElementSibling.style.display = 'none';
      selectedNode = undefined;
  }
  link
      .classed("link--target", false)
      .classed("link--source", false);
  node
      .classed("node--target", false)
      .classed("node--source", false)
      .classed("node--selected", false);
}

function mouseclicked(d) {
  node
      .each(function(n) { n.target = n.source = false; });

  link
      .classed("link--target", function(l) { if (l.target === d) return l.source.source = true; })
      .classed("link--source", function(l) { if (l.source === d) return l.target.target = true; })
    .filter(function(l) { return l.target === d || l.source === d; })
      .raise();

  node
      .classed("node--target", function(n) { return n.target; })
      .classed("node--source", function(n) { return n.source; })
      .classed("node--selected", function(n) { return n === d; });
}

// Lazily construct the package hierarchy from class names.
function packageHierarchy(classes) {
  var map = {};

  function find(name, tag, data) {
    var node = map[name], i;
    if (!node) {
      node = map[name] = data || {name: name, children: []};
      if (name.length) {
        node.parent = find(tag, tag.substring(0, tag.lastIndexOf(".")));
        node.parent.children.push(node);
        node.key = name;
      }
    }
    return node;
  }

  classes.forEach(function(d) {
    find(d.name, d.tag, d);
  });

  return d3.hierarchy(map[""]);
}

// Return a list of deps for the given array of nodes.
function packageDepends(nodes) {
  var map = {},
      depends = [];

  // Compute a map from name to node.
  nodes.forEach(function(d) {
    map[d.data.name] = d;
  });

  // For each dep, construct a link from the source to target node.
  nodes.forEach(function(d) {
    if (d.data.depends) d.data.depends.forEach(function(i) {
      var l = map[d.data.name].path(map[i]);
      l.allow = true;
      depends.push(l);
    });
    if (d.data.violates.length) {
      map[d.data.name].not_allow = true;
      d.data.violates.forEach(function(i) {
        map[i].not_allow = true;
        var l = map[d.data.name].path(map[i]);
        l.allow = false;
        depends.push(l);
      });
    }
  });

  return [depends, map];
}

show_result(dep_data, violate_libs);