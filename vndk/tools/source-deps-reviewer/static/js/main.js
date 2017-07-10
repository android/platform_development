var ccounter = 0;
var counter = 0;

function getSelText() {
  let txt = '';
  if (window.getSelection) { 
    txt = window.getSelection(); 
  }
  else if (document.getSelection) {
    txt = document.getSelection(); 
  }
  else if (document.selection) {
    txt = document.selection.createRange().text; 
  }
  else return;
  document.aform.selectedtext.value =  txt;
  $('#code_file_path').val($('#browsing_file_path').text());
  return txt;
}


$(function() { 
  $('#add').on('click', enter_task); 
});
$(function() {
  $('#add_code').on('click', enter_code); 
});
$(function() {
  $('#save_all').on('click', save_all); 
});

function enter_task () {
  let text = $('#enter_deps').val();
  $('#deps_list').append('<li><span class="display" id="dep' + counter + '">' +
      text + '</span>' + '<input type="text" class="edit" style="display:none"/>' +
      '<input type="submit" class="delete" value="X">' +'</li>');
  $('.delete').click(function(){ $(this).parent().remove(); });
  counter++;
};

function set_task(deps) {
  $('#deps_list').empty();
  counter = 0;
  let len = deps.length;
  for(let i = 0; i < len; i++) {
    let text = deps[i];
    $('#deps_list').append('<li><span class="display" id="dep' + counter +
        '">'+ text + '</span>' + '<input type="text" class="edit" style="display:none"/>' +
        '<input type="submit" class="delete" value="X">' +'</li>');
    $('.delete').click(function(){ $(this).parent().remove(); });
    counter++;
  }
}

document.getElementById("enter_deps").addEventListener("keydown", function(e) {
  if (!e) {
    let e = window.event; 
  }
  if (e.keyCode == 13) {
    enter_task();
  }
}, false);

function enter_code () {
  let text = $('#code_file_path').val() + ':' + $('#selected_text').val();
  $('#code_list').append('<li><span id="code' + ccounter + '">'+ text +
      '</span><input type="submit" class="delete" value="X">' +'</li>');
  $('.delete').click(function(){ $(this).parent().remove(); });
  ccounter++;
};

function set_code(codes) {
  $('#code_list').empty();
  ccounter = 0;
  let len = codes.length;
  for(let i = 0; i < len; i++) {
    let text = codes[i];
    $('#code_list').append('<li><span id="code' + ccounter + '">'+ text +
        '</span><input type="submit" class="delete" value="X">' +'</li>');
    $('.delete').click(function(){ $(this).parent().remove(); });
    ccounter++;
  }
}

function save_all() {
  let path = $('#file_path').text();
  let line_no = $('#line_no').text();
  console.log(path);
  deps = new Array();
  for(let i=0;i<counter;i++) {
    if ($('#dep' + i).length) {
      deps.push( $('#dep' + i).text() );
    }
  }
  codes = new Array();
  for(let i=0;i<ccounter;i++) {
    if ($('#code' + i).length) {
      codes.push( $('#code' + i).text() );
    }
  }
  $.getJSON('/save_all', {
      path: path+':'+line_no,
      deps: JSON.stringify(deps),
      codes: JSON.stringify(codes)
  }, function(data) {
      console.log(data.result);
  });
}

window.onload=function() {
  $.getJSON('/get_started', {
  }, function(data) {
    let lst = JSON.parse(data.lst);
    let done = JSON.parse(data.done);
    let len = done.length;
    for(let i=0;i<len;i++) {
      if (done[i]) {
        $('#item_list').append('<pre><a class="list-group-item list-group-item-success"' + 
                               ' onclick="set_item(this)">' + lst[i] + '</a></pre>');
      }
      else {
        $('#item_list').append('<pre><a class="list-group-item list-group-item-danger"' + 
                               'onclick="set_item(this)">' + lst[i] + '</a></pre>');
      }
    }
  });
};

function reload_js(src) {
  $('script[src="' + src + '"]').remove();
  $('<script>').attr('src', src).appendTo('head');
}

function set_browsing_file(path) {
  $('#browsing_file_path').text(path);
  $.getJSON('/get_file', {
    path: path
    }, function(data) {
      $('#browsing_file').children().first().text(data.result);
      reload_js('static/prism/js/prism.js');
  });
}


function set_highlight_line(line_no) {
  $('#browsing_file').attr('data-line', line_no);
}

function set_item(item) {
  // get all the inputs into an array.
  let name = $(item).text().split(':');
  let file = name[0];
  let line_no = name[1];
  $('#file_path').text(file);
  $('#line_no').text(line_no);

  $.getJSON('/load_file', {
    path: $(item).text()
  }, function(data) {
    let deps = JSON.parse(data.deps);
    let codes = JSON.parse(data.codes);
    set_task(deps);
    set_code(codes);
  });

  set_browsing_file(file);
  set_highlight_line(line_no);
  $('#selected_text').val('');
  $('#code_file_path').val('');

  return false;
};


$('#go_form').submit(function() {
  // get all the inputs into an array.
  let $inputs = $('#go_form :input');
  let values = {};
  $inputs.each(function() {
    values[this.name] = $(this).val();
  });
  console.log(values['browsing_path']);
  path = $('input[name="browsing_path"]').val();
  set_browsing_file(path);
  return false;
});
