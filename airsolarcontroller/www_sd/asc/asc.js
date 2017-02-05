<!-- scripts for data update and commands transmission -->
$(document).ready(function() {
  // init -- start the auto-updated functions
  // we should test here 'version' for the arduino sktech (hm or asc)
  // compatibility
  myTimer();        // start time -- auto updated every 1s
  updateContent();  // update the data -- auto updated every 8s
//updateswitches(); // update switches -- auto update every 10s ???

  // update info once
  updateParam();    // update fixed parameters
  updateOptions();
  
  // adapt the UI to SYSTEM
  checksystem();
  
  // events
  // on access to the nav bar => refresh all
  // !!! if you call again an auto-updated function !!!
  $("ul").click(function() {
	checksystem();
    updateOptions();
    updateParam();
  })
  
  // do a reset to default
  $("#reset_def").click(function() {
    putkeyval( "request", "default" );
  })
});

function myTimer() {
  var d = new Date();
  var t = d.toLocaleTimeString();
  var dd = d.toDateString();
  document.getElementById("clock").innerHTML = dd+" - "+t;
  setTimeout( function(){ myTimer() }, 1000); // redo in 1 sec    
}

function checksystem() {
  // test SYSTEM option
  // we should test 'version' for the arduino sktech (hm or asc)
  // compatibility
  $.getJSON("/data/get", function(data, status) {
	// why the /data/get/system request does not work??? 
    if ( parseInt(data.value.system) == 1 ) {
      // SYSTEM1
      // hide the solar heater parameters -- select with class="solar"
      $(".solar").hide();
      //alert("SYSTEM==1 option detected");
    } else {
      // SYSTEM2, i.e. everything else
      // show the solar heater parameters -- select with class="solar"
      $(".solar").show();
      //alert("SYSTEM==2 option detected");
   }  
  })	
}

function updateContent() {
  $.getJSON("/data/get/", function(data, status) {
    // update temperatures
    // only measures should be updates periodically => other info in updateParam()
    $('#tamb').html(data.value.tamb);
    $('#tset').html(data.value.tset);
    $('#hamb').html(data.value.hamb);
    $('#tcol').html(data.value.tcol);
    $('#text').html(data.value.text);
    $('#tusr1').html(data.value.tusr1);
    $('#tusr2').html(data.value.tusr2);

    // status & modes
    $('#statemh').html(data.value.statemh);
    $('#modemh').html(data.value.modemh);
    $('#statesh').html(data.value.statesh);

    // index
    $('#tcmh').html(data.value.tcmh);
    $('#tcsh').html(data.value.tcsh);
    $('#indsh').html(data.value.indsh);

    // status & modes -- explicit
    switch(parseInt(data.value.modemh)) {
      case 0:
        $("#x_modemh").html( "OFF" );
        break;
      case 1:
        $("#x_modemh").html( "HEAT" );
        break;
      case 2:
        $("#x_modemh").html( "ECO" );
        break;
      case 3:
        $("#x_modemh").html( "Freeze Protect" );
        break;
      default:
        //default code block
    }   
		
    switch(parseInt(data.value.modesh)) {
      case 0:
        $("#x_modesh").html( "OFF" );
        break;
      case 1:
        $("#x_modesh").html( "HEAT" );
        break;
      default:
        //default code block
    }   

    switch(parseInt(data.value.statemh)) {
      case 0:
        $("#x_statemh").html( "OFF" );
        break;
      case 1:
        $("#x_statemh").html( "ON" );
        break;
      default:
        //default code block
    }   

    switch(parseInt(data.value.statesh)) {
      case 0:
        $("#x_statesh").html( "OFF" );
        break;
      case 1:
        $("#x_statesh").html( "ON" );
        break;
      default:
        //default code block
    }   

	// update the ECO option
    $('#mhdteco>#x_eco2').prop('selected', false );
    $('#mhdteco>#x_eco3').prop('selected', false );
    $('#mhdteco>#x_eco5').prop('selected', false );
    $('#mhdteco>#x_eco10').prop('selected', false );

	// find the selected value
	// what todo if not within the list???
    switch(parseInt(data.value.dteco)) {
      case 2:
        $('#mhdteco>#x_eco2').prop('selected', true );
        break;
      case 3:
        $('#mhdteco>#x_eco3').prop('selected', true );
        break;
      case 5:
        $('#mhdteco>#x_eco5').prop('selected', true );
        break;
      case 10:
        $('#mhdteco>#x_eco10').prop('selected', true );
        break;
      default:
        //default code block
    }  
})
  setTimeout( function(){updateContent()}, 8000); // redo in 8 sec
}

function updateParam() {
  $.getJSON("/data/get/", function(data, status) {
    // update parameters
    // note: an id may only be used once => add p_* for this list
    // parameters list -- see arduino code
    $('#p_tamb').val(data.value.tamb);
    $('#p_hamb').val(data.value.hamb);
    $('#p_tcol').val(data.value.tcol);
    $('#p_text').val(data.value.text);
    $('#p_tusr1').val(data.value.tusr1);
    $('#p_tusr2').val(data.value.tusr2);

    $('#p_nloops').val(data.value.nloops);
    $('#p_psolth').val(data.value.psolth);
    $('#p_swsh').val(data.value.swsh);
    $('#p_swmh').val(data.value.swmh);
    $('#p_swusr').val(data.value.swusr);
    $('#p_statesh').val(data.value.statesh);
    $('#p_statemh').val(data.value.statemh);
    $('#p_statectrl').val(data.value.statectrl);
    $('#p_errsensor').val(data.value.errsensor);
    $('#p_errctrl').val(data.value.errctrl);
    $('#p_indsh').val(data.value.indsh);
    $('#p_tcsh').val(data.value.tcsh);
    $('#p_tcmh').val(data.value.tcmh);

    $('#p_tset').val(data.value.tset);
    $('#p_dteco').val(data.value.dteco);
    $('#p_modemh').val(data.value.modemh);
    $('#p_modesh').val(data.value.modesh);
    $('#p_modectrl').val(data.value.modectrl);
	
    $('#p_tmhon').val(data.value.tmhon);
    $('#p_tmhoff').val(data.value.tmhoff);
    $('#p_tshon').val(data.value.tshon);
    $('#p_tshoff').val(data.value.tshoff);
    $('#p_dtshon').val(data.value.dtshon);
    $('#p_dtshoff').val(data.value.dtshoff);
    $('#p_aug1').val(data.value.aug1);
    $('#p_aug2').val(data.value.aug2);
    $('#p_tfp').val(data.value.tfp);
    $('#p_hyst').val(data.value.hyst);
    $('#p_dtdht').val(data.value.dtdht);
    $('#p_vfan').val(data.value.vfan);
    $('#p_asolt').val(data.value.asolt);
    $('#p_system').val(data.value.system);
    $('#p_conf1').val(data.value.conf1);

    $('#p_version').val(data.value.version);

    $('#sltset').val(data.value.tset); // for the slider...
  })
}

function updateOptions() {
  $.getJSON("/data/get/", function(data, status) {
    // controller modes
    // update the checked options
    // main heater
    $('#sltset').val(data.value.tset);

    // update the main heater checkboxradio
    $("#mhoff").prop('checked', false ).checkboxradio( "refresh" );
    $("#mhheat").prop('checked', false ).checkboxradio( "refresh" );
    $("#mheco").prop('checked', false ).checkboxradio( "refresh" );
    $("#mhfp").prop('checked', false ).checkboxradio( "refresh" );
	
    switch(parseInt(data.value.modemh)) {
      case 0:
        $("#mhoff").prop('checked', true ).checkboxradio( "refresh" );
        break;
      case 1:
        $("#mhheat").prop('checked', true ).checkboxradio( "refresh" );
        break;
      case 2:
        $("#mheco").prop('checked', true ).checkboxradio( "refresh" );
        break;
      case 3:
        $("#mhfp").prop('checked', true ).checkboxradio( "refresh" );
        break;
      default:
        //default code block
    }
	
    // update the solar heater checkboxradio
    $("#shoff").prop('checked', false ).checkboxradio( "refresh" );
    $("#shheat").prop('checked', false ).checkboxradio( "refresh" );
	
    switch(parseInt(data.value.modesh)) {
      case 0:
        $("#shoff").prop('checked', true ).checkboxradio( "refresh" );
        break;
      case 1:
        $("#shheat").prop('checked', true ).checkboxradio( "refresh" );
        break;
      default:
        //default code block
    }
  });
}

// synchronize the switches status in html page
// we have to define the switches list here
function updateSwitches() {
  $.getJSON("/data/get/", function(data, status) {
    // declare the switches list
    updateSwitch("swusr", data.value.swusr);
  });
  // error handle for $.getJSON
  // see http://stackoverflow.com/questions/1740218/error-handling-in-getjson-calls
  // -- not implemented
  
  setTimeout( function(){updateSwitches()}, 10000); // redo in 10 sec
}

function updateSwitch( id, state ) {
  // refresh the current state of the switch
  // see https://api.jquerymobile.com/
  if ( state == 1 ) {
    // switch is on
    $('#'+id).prop('checked', true ).flipswitch( "refresh" );
  } else {
    // switch is off
    $('#'+id).prop('checked', false ).flipswitch( "refresh" );
  }
}

// Function to control relays
function switchClick(clicked_id, clicked_chck) {
  // build the REST request from id & chck
  cmdtext = "/data/put/"+clicked_id;
  
  if ( clicked_chck == false ) {
    cmdtext = cmdtext+"/0";
  } else {
    cmdtext = cmdtext+"/1";
  }

  // send the command with a json request
  $.getJSON(cmdtext, function(data, status) {
    // should do some indication of the success => status...
  });
}

// put key value on arduino
function putkeyval( key, value ) {
  // build the REST request
  cmdtext = "/data/put/"+ key +"/"+value;

  // send the command with a json request
  $.getJSON(cmdtext, function(data, status) {
    // should do some indication of the success => status...
  });
}

function errorHandle( data, status ) {
  if ( status != 'success') {
    // just tell
    alert("Command failed!");
  }
}