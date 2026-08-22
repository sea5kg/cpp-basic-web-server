
function test_api() {
      $.ajax({
        url: "/api", // Server endpoint URL
        method: "POST",            // HTTP method (GET, POST, PUT, DELETE, etc.)
        dataType: "json",          // Expected data type in the response (xml, json, script, html)
        contentType: "application/json",
        data: JSON.stringify({
            method: "John",
            location: "Boston"
        }),
        success: function(data) {  // Function to run if the request succeeds
            console.log("Data received:", data);
            // Process the data here (e.g., update UI)
        },
        error: function(jqXHR, textStatus, errorThrown) { // Function to run if an error occurs
            alert("Error: " + textStatus + " : " + errorThrown);
        },
        complete: function(jqXHR, status) { // Function to run when the request finishes (after success or error)
            console.log("Request finished.");
        }
    });
}