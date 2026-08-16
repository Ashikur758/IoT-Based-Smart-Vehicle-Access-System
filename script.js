// ======================================================
// Public GitHub Dashboard JavaScript
// ======================================================

// ------------------------------------------------------
// Carousel Images
// ------------------------------------------------------

const CAROUSEL_IMAGES = [
  "project-01.jpg",
  "project-02.jpg",
  "project-03.jpg"
];

const CAROUSEL_CAPTIONS = [
  "Smart Vehicle Access System",
  "Hardware Prototype",
  "IoT Dashboard"
];

// ------------------------------------------------------
// Carousel
// ------------------------------------------------------

(function initCarousel() {

  const wrapper = document.getElementById("carousel-wrapper");
  const dotsBox = document.getElementById("carousel-dots");

  let current = 0;

  const slideEls = CAROUSEL_IMAGES.map((src, i) => {

    const slide = document.createElement("div");

    slide.className =
      "carousel-slide" +
      (i === 0 ? " active" : "");

    slide.innerHTML = `
      <img src="${src}" alt="project-${i + 1}">
      <div class="carousel-caption">
        ${CAROUSEL_CAPTIONS[i] || ""}
      </div>
    `;

    wrapper.appendChild(slide);

    return slide;
  });

  CAROUSEL_IMAGES.forEach((_, i) => {

    const dot = document.createElement("span");

    if (i === 0) {
      dot.classList.add("active");
    }

    dotsBox.appendChild(dot);
  });

  const dotEls = Array.from(dotsBox.children);

  function goToSlide(nextIndex) {

    slideEls.forEach(slide => {
      slide.classList.remove("active", "prev");
    });

    if (slideEls[current]) {
      slideEls[current].classList.add("prev");
    }

    if (slideEls[nextIndex]) {
      slideEls[nextIndex].classList.add("active");
    }

    if (dotEls[current]) {
      dotEls[current].classList.remove("active");
    }

    if (dotEls[nextIndex]) {
      dotEls[nextIndex].classList.add("active");
    }

    current = nextIndex;
  }

  if (slideEls.length > 1) {

    setInterval(() => {
      goToSlide(
        (current + 1) % slideEls.length
      );
    }, 5000);

  }

})();

// ======================================================
// Firebase
// ======================================================

import(
  "https://www.gstatic.com/firebasejs/10.12.0/firebase-app.js"
).then(async ({ initializeApp }) => {

  const {
    getDatabase,
    ref,
    onValue,
    onChildAdded,
    get,
    query,
    limitToLast
  } = await import(
    "https://www.gstatic.com/firebasejs/10.12.0/firebase-database.js"
  );

  // ====================================================
  // PUBLIC TEMPLATE
  // ====================================================
  // Add your own Firebase Web App configuration here.
  // Do NOT add Wi-Fi passwords or private credentials.
  // ====================================================

  const firebaseConfig = {

    apiKey:
      "YOUR_FIREBASE_API_KEY",

    authDomain:
      "YOUR_FIREBASE_PROJECT.firebaseapp.com",

    databaseURL:
      "YOUR_FIREBASE_DATABASE_URL",

    projectId:
      "YOUR_FIREBASE_PROJECT_ID",

    storageBucket:
      "YOUR_FIREBASE_STORAGE_BUCKET",

    messagingSenderId:
      "YOUR_FIREBASE_MESSAGING_SENDER_ID",

    appId:
      "YOUR_FIREBASE_APP_ID"
  };

  const app = initializeApp(firebaseConfig);

  const database = getDatabase(app);

  // ====================================================
  // LIVE CLOCK
  // ====================================================

  function formatDate(date) {

    return date.toLocaleDateString(
      "en-US",
      {
        weekday: "short",
        month: "short",
        day: "numeric",
        year: "numeric"
      }
    );

  }

  function formatTime(date) {

    return date.toLocaleTimeString(
      "en-US",
      {
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
        hour12: true
      }
    );

  }

  function updateClock() {

    const now = new Date();

    document.getElementById(
      "live-time"
    ).innerText = formatTime(now);

    document.getElementById(
      "live-date"
    ).innerText = formatDate(now);

  }

  setInterval(updateClock, 1000);

  updateClock();

  // ====================================================
  // FIREBASE CONNECTION STATUS
  // ====================================================

  const connectedRef =
    ref(database, ".info/connected");

  onValue(
    connectedRef,
    snapshot => {

      const element =
        document.getElementById("conn-status");

      if (snapshot.val() === true) {

        element.innerText = "Live";

        element.className =
          "connection-status status-connected";

      } else {

        element.innerText = "Disconnected";

        element.className =
          "connection-status status-disconnected";

      }

    }
  );

  // ====================================================
  // OVERVIEW
  // ====================================================

  const overviewRef =
    ref(database, "overview");

  onValue(
    overviewRef,
    snapshot => {

      const data = snapshot.val();

      if (!data) {
        return;
      }

      document.getElementById(
        "total-capacity"
      ).innerText =
        data.total_capacity ?? 10;

      document.getElementById(
        "vehicles-inside"
      ).innerText =
        data.inside ?? 0;

      document.getElementById(
        "total-entry"
      ).innerText =
        data.total_entry ?? 0;

      document.getElementById(
        "total-exit"
      ).innerText =
        data.total_exit ?? 0;

    }
  );

  // ====================================================
  // VEHICLE LOGS
  // ====================================================

  let allLogs = [];

  let initialLoadDone = false;

  const clientArrivalStamp = {};

  const logsQuery =
    query(
      ref(database, "vehicle_logs"),
      limitToLast(100)
    );

  get(logsQuery)
    .then(snapshot => {

      const initial = [];

      snapshot.forEach(childSnap => {

        initial.unshift({
          key: childSnap.key,
          ...childSnap.val()
        });

      });

      allLogs = initial;

      initialLoadDone = true;

      calculateCategoryInsideBreakdown();

      renderLogsTable();

      // ----------------------------------------------
      // New vehicle log listener
      // ----------------------------------------------

      onChildAdded(
        logsQuery,
        childSnap => {

          if (!initialLoadDone) {
            return;
          }

          const key = childSnap.key;

          if (
            allLogs.some(
              log => log.key === key
            )
          ) {
            return;
          }

          const now = new Date();

          clientArrivalStamp[key] = {

            formattedDate:
              formatDate(now),

            formattedTime:
              formatTime(now),

            isoDate:
              now.toISOString()
                .split("T")[0]

          };

          allLogs.unshift({
            key,
            ...childSnap.val()
          });

          if (allLogs.length > 100) {
            allLogs.pop();
          }

          calculateCategoryInsideBreakdown();

          renderLogsTable();

        }
      );

    })
    .catch(error => {

      console.error(
        "Failed to load vehicle logs:",
        error
      );

    });

  // ====================================================
  // CATEGORY INSIDE BREAKDOWN
  // ====================================================

  function calculateCategoryInsideBreakdown() {

    const latestVehicleState = {};

    allLogs.forEach(log => {

      const vehicleKey =
        (
          log.vehicle_no ||
          log.name ||
          ""
        )
        .trim()
        .toLowerCase();

      if (
        vehicleKey &&
        !latestVehicleState[vehicleKey]
      ) {

        latestVehicleState[vehicleKey] = log;

      }

    });

    let teacherCount = 0;

    let studentStaffCount = 0;

    let guestCount = 0;

    Object.values(
      latestVehicleState
    ).forEach(log => {

      if (
        (log.access || "")
          .toLowerCase() !== "entry"
      ) {
        return;
      }

      const category =
        (
          log.category ||
          ""
        ).toLowerCase();

      if (
        category.includes("teacher")
      ) {

        teacherCount++;

      } else if (
        category.includes("student") ||
        category.includes("staff")
      ) {

        studentStaffCount++;

      } else {

        guestCount++;

      }

    });

    document.getElementById(
      "teachers-inside"
    ).innerText = teacherCount;

    document.getElementById(
      "students-inside"
    ).innerText = studentStaffCount;

    document.getElementById(
      "guests-inside"
    ).innerText = guestCount;

  }

  // ====================================================
  // DATE / TIME PARSER
  // ====================================================

  function parseDateTime(log) {

    if (
      log.key &&
      clientArrivalStamp[log.key]
    ) {

      return clientArrivalStamp[log.key];

    }

    let dateObj = null;

    // ----------------------------------------------
    // Firebase timestamp
    // ----------------------------------------------

    if (log.timestamp) {

      const timestamp =
        Number(log.timestamp);

      if (
        !isNaN(timestamp) &&
        timestamp > 0
      ) {

        dateObj =
          new Date(
            timestamp > 100000000000
              ? timestamp
              : timestamp * 1000
          );

      } else {

        const parsed =
          new Date(log.timestamp);

        if (!isNaN(parsed.getTime())) {
          dateObj = parsed;
        }

      }

    }

    // ----------------------------------------------
    // Date + Time fields
    // ----------------------------------------------

    if (
      (!dateObj ||
        isNaN(dateObj.getTime())) &&
      (log.date || log.time)
    ) {

      const parsed =
        new Date(
          `${log.date || new Date()
            .toISOString()
            .split("T")[0]} ${
              log.time || ""
          }`.trim()
        );

      if (!isNaN(parsed.getTime())) {
        dateObj = parsed;
      }

    }

    // ----------------------------------------------
    // Return formatted date/time
    // ----------------------------------------------

    if (
      dateObj &&
      !isNaN(dateObj.getTime())
    ) {

      return {

        formattedDate:
          formatDate(dateObj),

        formattedTime:
          formatTime(dateObj),

        isoDate:
          dateObj
            .toISOString()
            .split("T")[0]

      };

    }

    return {

      formattedDate:
        log.date || "N/A",

      formattedTime:
        log.time || "N/A",

      isoDate:
        log.date || ""

    };

  }

  // ====================================================
  // RENDER VEHICLE LOG TABLE
  // ====================================================

  function renderLogsTable() {

    const searchVal =
      document.getElementById(
        "search-input"
      ).value
        .toLowerCase()
        .trim();

    const categoryVal =
      document.getElementById(
        "filter-category"
      ).value;

    const accessVal =
      document.getElementById(
        "filter-access"
      ).value;

    const dateVal =
      document.getElementById(
        "filter-date"
      ).value;

    const tableBody =
      document.getElementById(
        "logs-table"
      );

    tableBody.innerHTML = "";

    const knownCategories = [
      "teacher",
      "student",
      "staff",
      "guest teacher",
      "guest"
    ];

    const filteredLogs =
      allLogs.filter(log => {

        const name =
          (log.name || "")
            .toLowerCase();

        const vehicleNo =
          (log.vehicle_no || "")
            .toLowerCase();

        const matchesSearch =
          name.includes(searchVal) ||
          vehicleNo.includes(searchVal);

        const logCategory =
          (log.category || "")
            .toLowerCase();

        const matchesCategory =
          categoryVal === "All"
            ? true
            : categoryVal === "Other"
              ? !knownCategories.some(
                  item =>
                    logCategory.includes(item)
                )
              : logCategory.includes(
                  categoryVal.toLowerCase()
                );

        const matchesAccess =
          accessVal === "All" ||
          (log.access || "")
            .toLowerCase() ===
            accessVal.toLowerCase();

        const dateTime =
          parseDateTime(log);

        const matchesDate =
          !dateVal ||
          dateTime.isoDate === dateVal;

        return (
          matchesSearch &&
          matchesCategory &&
          matchesAccess &&
          matchesDate
        );

      });

    // ----------------------------------------------
    // No result
    // ----------------------------------------------

    if (filteredLogs.length === 0) {

      tableBody.innerHTML = `
        <tr>
          <td colspan="8" class="no-result">
            No matching records found
          </td>
        </tr>
      `;

      return;

    }

    // ----------------------------------------------
    // Add rows
    // ----------------------------------------------

    filteredLogs.forEach(log => {

      const dateTime =
        parseDateTime(log);

      const isEntry =
        (log.access || "")
          .toLowerCase() === "entry";

      const row =
        document.createElement("tr");

      row.innerHTML = `

        <td>
          ${dateTime.formattedDate}
        </td>

        <td>
          ${dateTime.formattedTime}
        </td>

        <td>
          <b>
            ${log.name || "N/A"}
          </b>
        </td>

        <td>
          ${log.vehicle_no || "N/A"}
        </td>

        <td>
          ${log.type || "N/A"}
        </td>

        <td>
          ${log.category || "N/A"}
        </td>

        <td>
          <span class="badge ${
            isEntry
              ? "badge-entry"
              : "badge-exit"
          }">
            ${log.access || ""}
          </span>
        </td>

        <td>
          <span class="badge badge-status">
            ${log.status || ""}
          </span>
        </td>

      `;

      tableBody.appendChild(row);

    });

  }

  // ====================================================
  // SEARCH
  // ====================================================

  document
    .getElementById("search-input")
    .addEventListener(
      "input",
      renderLogsTable
    );

  // ====================================================
  // CATEGORY FILTER
  // ====================================================

  document
    .getElementById("filter-category")
    .addEventListener(
      "change",
      renderLogsTable
    );

  // ====================================================
  // ACCESS FILTER
  // ====================================================

  document
    .getElementById("filter-access")
    .addEventListener(
      "change",
      renderLogsTable
    );

  // ====================================================
  // DATE FILTER
  // ====================================================

  document
    .getElementById("filter-date")
    .addEventListener(
      "change",
      renderLogsTable
    );

});