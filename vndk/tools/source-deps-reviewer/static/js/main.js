//$SCRIPT_ROOT = {{ request.script_root|tojson|safe }};

function getSelText() {
    var txt = '';
     if (window.getSelection) { txt = window.getSelection(); }
    else if (document.getSelection) { txt = document.getSelection(); }
    else if (document.selection) { txt = document.selection.createRange().text; }
    else return;
    document.aform.selectedtext.value =  txt;
    
    //console.log($('#browsing_file_path').text());
    $('#code_file_path').val($('#browsing_file_path').text());

    return txt;
}

var ccounter = 0;
var counter = 0;


/*
$(".display").click(function(){ $(this).hide().siblings(".edit").show().val($(this).text()).focus(); }); 
$('.delete').click(function(){ $(this).parent().remove(); }); 
$('.edit').focusout(function(){ $(this).hide().siblings(".display").show().text($(this).val()); }); 
*/

$(function() { $('#add').on('click', enter_task); });
$(function() { $('#add_code').on('click', enter_code); });
$(function() { $('#save_all').on('click', save_all); });

function enter_task () {
     var text = $('#enter_deps').val(); 

    $('#deps_list').append('<li><span class="display" id="dep' + counter + '">'+ text + '</span>' + '<input type="text" class="edit" style="display:none"/>' + '<input type="submit" class="delete" value="X">' +'</li>'); 
    $('.delete').click(function(){ $(this).parent().remove(); }); 
    counter++; 
}; 

function set_task(deps) {
    $('#deps_list').empty();
    counter = 0;
    var len = deps.length;
    for(i = 0; i < len; i++) {
        var text = deps[i];
        $('#deps_list').append('<li><span class="display" id="dep' + counter + '">'+ text + '</span>' + '<input type="text" class="edit" style="display:none"/>' + '<input type="submit" class="delete" value="X">' +'</li>'); 
        $('.delete').click(function(){ $(this).parent().remove(); }); 
        counter++; 
    }
}

document.getElementById("enter_deps").addEventListener("keydown", function(e) {
    if (!e) { var e = window.event; }
    //e.preventDefault(); // sometimes useful
    // Enter is pressed
    if (e.keyCode == 13) {enter_task();}
}, false);

function enter_code () {
    // var text = getSelText();
    var text = $('#code_file_path').val() + ':' + $('#selected_text').val();
    $('#code_list').append('<li><span id="code' + ccounter + '">'+ text + '</span><input type="submit" class="delete" value="X">' +'</li>'); 
    $('.delete').click(function(){ $(this).parent().remove(); }); 
    ccounter++; 
}; 

function set_code(codes) {
    $('#code_list').empty();
    ccounter = 0;
    var len = codes.length;
    for(i = 0; i < len; i++) {
        var text = codes[i];
        $('#code_list').append('<li><span id="code' + ccounter + '">'+ text + '</span><input type="submit" class="delete" value="X">' +'</li>'); 
        $('.delete').click(function(){ $(this).parent().remove(); }); 
        ccounter++; 
    }
}

function save_all() {
    var path = $('#file_path').text();
    var line_no = $('#line_no').text();
    console.log(path);
    deps = new Array();
    for(i=0;i<counter;i++) {
        if ($('#dep' + i).length) 
            deps.push( $('#dep' + i).text() );
    }
    codes = new Array();
    for(i=0;i<ccounter;i++) {
        if ($('#code' + i).length) 
            codes.push( $('#code' + i).text() );
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
      var lst = JSON.parse(data.lst);
      var done = JSON.parse(data.done);
      var len = done.length;

      for(i=0;i<len;i++) {
          if (done[i])
              $('#item_list').append('<a class="list-group-item list-group-item-success" onclick="set_item(this)">' + lst[i] + '</a>');
          else
              $('#item_list').append('<a class="list-group-item list-group-item-danger" onclick="set_item(this)">' + lst[i] + '</a>');
      }
      
  });

}; 

/*
function reloadStylesheets() {
    var queryString = '?reload=' + new Date().getTime();
    $('link[rel="stylesheet"]').each(function () {
        this.href = this.href.replace(/\?.*|$/, queryString);
    });
}
*/
function reload_js(src) {
     $('script[src="' + src + '"]').remove();
     $('<script>').attr('src', src).appendTo('head');
 }

function set_browsing_file(path) {
    $('#browsing_file_path').text(path);
    $.getJSON('/get_file', {
        path: path
        }, function(data) {
          $('#browsing_file').text(data.result);
          reload_js('static/js/prism.js');
    });
}


function set_item(item) {
    // get all the inputs into an array.
    var name = $(item).text().split(':');
    var file = name[0];
    var line_no = name[1];
    $('#file_path').text(file);
    $('#line_no').text(line_no);

    $.getJSON('/load_file', {
        path: $(item).text()
    }, function(data) {
        var deps = JSON.parse(data.deps);
        var codes = JSON.parse(data.codes);
        // load deps here
        set_task(deps);
        set_code(codes);

    });

    set_browsing_file(file);
    $('#selected_text').val('');
    $('#code_file_path').val('');

    return false;
};


$('#go_form').submit(function() {
    // get all the inputs into an array.
    var $inputs = $('#go_form :input');
    var values = {};
    $inputs.each(function() {
    
        values[this.name] = $(this).val();
    });
   console.log(values['browsing_path']);
    path = $('input[name="browsing_path"]').val();
    set_browsing_file(path);
    return false;
});
