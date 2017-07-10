(function () {
  'use strict';

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
    else {
      return;
    }
    document.aform.selectedtext.value =  txt;
    $('#code_file_path').val($('#browsing_file_path').text());
    return txt;
  }


  // counter is a global var
  function task_html(text, cnt) {
    return '<li><span class="display" id="dep' + cnt + '">' +
    text + '</span>' + '<input type="text" class="edit" style="display:none"/>' +
    '<input type="submit" class="delete" value="X">' +'</li>';
  }

  // ccounter is a global var
  function code_html(text, cnt) {
    return '<li><span id="code' + cnt + '">'+ text +
    '</span><input type="submit" class="delete" value="X">' +'</li>';
  }

  function item_html(done, text) {
    let atag = document.createElement('a');
    atag.innerText = text;
    if (done) {
      atag.className = "list-group-item list-group-item-success";
    } else {
      atag.className = "list-group-item list-group-item-danger";
    }
    let pretag = document.createElement('pre');
    pretag.appendChild(atag);
    pretag.onclick = set_item;
    return pretag;
  }

  function enter_task () {
    let text = $('#enter_deps').val();
    $('#deps_list').append(task_html(text, counter));
    $('.delete').click(function(){ $(this).parent().remove(); });
    counter++;
    return false;
  }

  function set_task(deps) {
    $('#deps_list').empty();
    counter = 0;
    let len = deps.length;
    for (let i = 0; i < len; i++) {
      let text = deps[i];
      $('#deps_list').append(task_html(text, counter));
      $('.delete').click(function(){ $(this).parent().remove(); });
      counter++;
    }
  }

  function enter_code () {
    let text = $('#code_file_path').val() + ':' + $('#selected_text').val();
    $('#code_list').append(code_html(text, ccounter));
    $('.delete').click(function(){ $(this).parent().remove(); });
    ccounter++;
    return false;
  }

  function set_code(codes) {
    $('#code_list').empty();
    ccounter = 0;
    let len = codes.length;
    for (let i = 0; i < len; i++) {
      let text = codes[i];
      $('#code_list').append(code_html(text, ccounter));
      $('.delete').click(function(){ $(this).parent().remove(); });
      ccounter++;
    }
  }

  $(document).ready(on_load);

  function on_load() {
    $.getJSON('/get_started', {
    }, function(data) {
      $('#item_list').empty();

      const lst = JSON.parse(data.lst);
      const done = JSON.parse(data.done);
      const pattern_lst = JSON.parse(data.pattern_lst);
      let len = done.length;
      for (let i = 0; i < len; i++) {
        $('#item_list').append( item_html(done[i], lst[i]) );
      }
      len = pattern_lst.length;
      for (let i = 0; i < len; i++) {
        $('#pattern_list').append('<li>' + pattern_lst[i] + '</li>');
      }
    });
  }

  function save_all() {
    let path = $('#file_path').text();
    let line_no = $('#line_no').text();
    console.log(path);
    console.log(line_no);
    console.log(counter);
    console.log(ccounter);
    let deps = new Array();
    for (let i = 0; i < counter; i++) {
      if ($('#dep' + i).length) {
        deps.push( $('#dep' + i).text() );
      }
    }
    let codes = new Array();
    for (let i = 0; i < ccounter; i++) {
      if ($('#code' + i).length) {
        codes.push( $('#code' + i).text() );
      }
    }

    if ( path == '' || line_no == '' ) {
      console.log('Nothing saved');
      return false;
    }
    $.getJSON('/save_all', {
      path: path+':'+line_no,
      deps: JSON.stringify(deps),
      codes: JSON.stringify(codes)
    }, function(data) {
      console.log(data.result);
      on_load();
    });
    return false;
  }

  function set_browsing_file(path) {
    $('#browsing_file_path').text(path);
    $.getJSON('/get_file', {
      path: path
    }, function(data) {
      $('#browsing_file').children().first().text(data.result);
      let obj = $('#browsing_file').children().first();
      Prism.highlightElement($('#code')[0]);
    });
  }

  function set_highlight_line(line_no) {
    $('#browsing_file').attr('data-line', line_no);
  }

  function set_item(Event) {
    // get all the inputs into an array.
    let item = Event.target;
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
  }

  $('#go_form').submit(function() {
    // get all the inputs into an array.
    const $inputs = $('#go_form :input');
    let values = {};
    $inputs.each(function() {
      values[this.name] = $(this).val();
    });
    console.log(values['browsing_path']);
    path = $('input[name="browsing_path"]').val();
    set_browsing_file(path);
    return false;
  });

  $('#add_pattern').submit(function() {
    const $inputs = $('#add_pattern :input');
    let values = {};
    $inputs.each(function() {
      values[this.name] = $(this).val();
    });
    console.log(values);
    $.getJSON('/add_pattern', {
      pattern: values['pattern'],
      is_regex: $('#add_pattern').children().eq(1).is(':checked')
    }, function(data) {
      //
    });
    return true;
  });

  $('#add_deps').submit(enter_task);
  $('#add_code').submit(enter_code);
  $('#save_all').submit(save_all);
  $('#get_selection').click(getSelText);

})();
